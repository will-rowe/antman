#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "cli/ketopt.h"
#include "config/config.h"
#include "daemonize/daemonize.h"
#include "log/slog.h"
#include "log/helpers.h"

#define DD_PROG_NAME "antman"
#define DD_VERSION "0.0.1"
#define DD_CONFIG "/Users/willrowe/Google Drive/code/c/projects/antman/.antman.config"
#define DD_WORKDIR "/Users/willrowe/Google Drive/code/c/projects/antman" // working directory for the daemon

/*
   printUsage prints the usage info for antman
*/
void printUsage(void) {
    printf("usage:\tantman [flags]\n\n"
        "flags:\n"
        "\t --start                \t start the antman daemon\n"
        "\t --stop                 \t stop the antman daemon\n"
        "\t --setWatchDir <path>   \t set the watch directory\n"
        "\n"
        "\t -h                     \t prints this help and exits\n"
        "\t -v                     \t prints version number and exits\n");
}

/*
   stopantman
*/
int stopantman(Config* ddconfig) {
    if (!ddconfig->running) {
        slog(0, SLOG_WARN, "daemon is already stopped, ignoring stop request");
    } else {
        slog(0, SLOG_INFO, "stopping the daemon...");
        slog(0, SLOG_INFO, "\t- daemon PID: %d", ddconfig->pid);
        kill(ddconfig->pid, SIGTERM);

        //TODO: instead of the above, use waitpid, then decide to use a SIGKILL and then harvest zombies

        // update the config
        ddconfig->pid = -1;
        ddconfig->running = false;
        if (writeConfig(ddconfig, ddconfig->configFile) != 0 ) {
            slog(0, SLOG_ERROR, "failed to update config file");
            return 1;
        }
    }
    return 0;
}

/*
   setantman is used to set the watch directory
*/
int setantman(Config* ddconfig, char* dirName) {
    slog(0, SLOG_INFO, "setting the watch directory...");

    // check directory exists and is accessible
    DIR* dir = opendir(dirName);
    if (dir) {
        closedir(dir);

        // update the config and re-write the file
        ddconfig->watchDir = dirName;
        if (writeConfig(ddconfig, ddconfig->configFile) != 0 ) {
            slog(0, SLOG_ERROR, "failed to update config file with new watch directory");
            return 1;
        } 
        slog(0, SLOG_INFO, "\t- directory set to: %s", dirName);
    } else if (ENOENT == errno) {
        slog(0, SLOG_ERROR, "specified directory does not exist: %s", dirName);
        return 1;
    } else {
        slog(0, SLOG_ERROR, "can't access the specified directory: %s", dirName);
        return 1;
    }
    return 0;
}

/*
   startantman
*/
void startantman(Config* ddconfig, char* logName) {
    if (ddconfig->running) {
        slog(0, SLOG_WARN, "daemon is already running, ignoring start request");
        slog(0, SLOG_INFO, "\t- daemon PID: %d", ddconfig->pid);
    } else {
        slog(0, SLOG_INFO, "starting the daemon...");
        slog(0, SLOG_INFO, "\t- changing working directory to: %s", DD_WORKDIR);
        slog(0, SLOG_INFO, "\t- redirecting antman log to file: %s/%s.log", DD_WORKDIR, logName);
        slog(0, SLOG_INFO, "\t- directory to watch: %s", ddconfig->watchDir);
        slog(0, SLOG_INFO, "donzo.");
        startDaemon(DD_PROG_NAME, DD_WORKDIR, ddconfig);
    }
    return;
}

/*
    main is the antman entry point
*/
int main(int argc, char *argv[]) {

    // set up the long flags
    static ko_longopt_t longopts[] = {
        { "start", ko_no_argument,       301 },
        { "stop", ko_no_argument,       302 },
        { "setWatchDir", ko_required_argument, 303 }
    };

    // set up the job list
    int start = 0, stop = 0;
    char* watchDir = "";

    // get the CLI info
    ketopt_t opt = KETOPT_INIT;
    int c;
    while ((c = ketopt(&opt, argc, argv, 1, "hvu:", longopts)) >= 0) {
        if (c == 'h') {
            printUsage();
            return 0;
        }
        else if (c == 'v') {
            printf("%s\n", DD_VERSION);
            return 0;
        }
        else if (c == 301) start = 1;
        else if (c == 302) stop = 1;
        else if (c == 303) opt.arg? (watchDir = opt.arg) : (watchDir = DD_DEFAULT_WATCH_DIR);
        else if (c == 'u') printf("unused flag:  -u %s\n", opt.arg);
        else if (c == '?') {
            fprintf(stderr, "unknown flag: -%c\n\n", opt.opt? opt.opt : ':');
            printUsage();
            return 1;
        }
        else if (c == ':') {
            fprintf(stderr, "missing arg with flag: -%c\n\n", opt.opt? opt.opt : ':');
            printUsage();
            return 1;
        }
    }

    // check we have a job to do, otherwise print the help screen and exit
    if (start + stop == 0 && (watchDir[0] == '\0')) {
        fprintf(stderr, "nothing to do: no flags set\n\n");
        printUsage();
        return 1;
    }

    // set up the log
    char logName[128];
    snprintf(logName, sizeof logName, "%s%s%s", DD_PROG_NAME, "-", getTimestamp());
    slog_init(logName, "log/slog.cfg", 4, 1);
    slog(0, SLOG_INFO, "starting antman (version: %s)", DD_VERSION);

    // all good so far, now make sure the config file exists and can be written to
    slog(0, SLOG_INFO, "loading the config file...");
    slog(0, SLOG_INFO, "\t- registered location: %s", DD_CONFIG);
    if (access(DD_CONFIG, F_OK) == -1) {
        slog(0, SLOG_WARN, "\t- config file doesn't exist");
        slog(0, SLOG_INFO, "\t- creating a new config file");
        Config *tmp = initConfig();
        if (writeConfig(tmp, DD_CONFIG) != 0 ) {
            slog(0, SLOG_ERROR, "\t- failed to create a new config file");
            exit(1);
        }
        destroyConfig(tmp);
    }
    if (access(DD_CONFIG, W_OK) == -1) {
        slog(0, SLOG_ERROR, "\t- no write access to the config file");
        return 1;
    }
    Config *ddconfig = initConfig();
    if (loadConfig(ddconfig, DD_CONFIG) != 0) {
        destroyConfig(ddconfig);
        slog(0, SLOG_ERROR, "\t- could not load config file, may be corrupted");
        return 1;
    }
    slog(0, SLOG_INFO, "\t- config loaded");

    // handle any stop request
    if (stop == 1) {
        if (stopantman(ddconfig) != 0) exit(1);
    }

    // handle any setWatchDir request
    if (watchDir[0] != '\0') {

        // if the daemon is already running, stop it, update the watch dir, then start the daemon again
        if (ddconfig->running) {
            if (stopantman(ddconfig) != 0) exit(1);
            if (setantman(ddconfig, watchDir) != 0) exit(1);
            startantman(ddconfig, logName);
        } else {
            if (setantman(ddconfig, watchDir) != 0) exit(1);
        }
    }

    // handle any start request
    if (start == 1) {
        startantman(ddconfig, logName);
    }
    slog(0, SLOG_INFO, "donzo.");
    return 0;
}
