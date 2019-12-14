#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "ketopt.h"
#include "config.h"
#include "daemonize.h"
#include "slog.h"
#include "helpers.h"

/*
   printUsage prints the usage info for antman
*/
void printUsage(void)
{
    printf("usage:\tantman [flags]\n\n"
           "flags:\n"
           "\t --start                \t start the antman daemon\n"
           "\t --stop                 \t stop the antman daemon\n"
           "\t --setWatchDir <path>   \t set the watch directory\n"
           "\t --getPID               \t return the PID of the antman daemon\n"
           "\n"
           "\t -h                     \t prints this help and exits\n"
           "\t -v                     \t prints version number and exits\n");
}

/*
  checkPID
   - checks if the antman daemon is running
   - then checks if the PID is correct
   - returns the PID of the antman daemon
*/
int checkPID(config_t *amConfig)
{

    // if running, check the PID
    if (amConfig->running)
    {
        // check it exists
        if (kill(amConfig->pid, 0) != 0)
        {
            fprintf(stderr, "error: the registered antman pid is not running\n\n");
            exit(1);
        }

        // TODO: check it PID name is antman
        // something like #ps -p $(./src/antman --getPID) -o comm=

        // return the PID
        return amConfig->pid;
    }

    // return -1 if no daemon is running
    else
    {
        return -1;
    }
}

/*
   stopAntman
*/
int stopAntman(config_t *amConfig)
{
    if (!amConfig->running)
    {
        slog(0, SLOG_WARN, "daemon is already stopped, ignoring stop request");
    }
    else
    {
        slog(0, SLOG_INFO, "stopping the daemon...");
        slog(0, SLOG_INFO, "\t- daemon PID: %d", amConfig->pid);
        if (kill(amConfig->pid, SIGTERM) != 0)
        {
            slog(0, SLOG_ERROR, "could not kill the daemon process");
            return 1;
        }

        //TODO: instead of the above, use waitpid, then decide to use a SIGKILL and then harvest zombies

        // update the config
        amConfig->pid = -1;
        amConfig->running = false;
        if (writeConfig(amConfig, amConfig->configFile) != 0)
        {
            slog(0, SLOG_ERROR, "failed to update config file");
            return 1;
        }
    }
    return 0;
}

/*
   setAntman is used to set the watch directory
*/
int setAntman(config_t *amConfig, char *dirName)
{
    slog(0, SLOG_INFO, "setting the watch directory...");

    // check directory exists and is accessible
    DIR *dir = opendir(dirName);
    if (dir)
    {
        closedir(dir);

        // update the config and re-write the file
        amConfig->watchDir = dirName;
        if (writeConfig(amConfig, amConfig->configFile) != 0)
        {
            slog(0, SLOG_ERROR, "failed to update config file with new watch directory");
            return 1;
        }
        slog(0, SLOG_INFO, "\t- directory set to: %s", dirName);
    }
    else if (ENOENT == errno)
    {
        slog(0, SLOG_ERROR, "specified directory does not exist: %s", dirName);
        return 1;
    }
    else
    {
        slog(0, SLOG_ERROR, "can't access the specified directory: %s", dirName);
        return 1;
    }
    return 0;
}

/*
   startAntman
*/
int startAntman(config_t *amConfig, char *logName)
{
    if (amConfig->running)
    {
        slog(0, SLOG_WARN, "daemon is already running, ignoring start request");
        slog(0, SLOG_INFO, "\t- daemon PID: %d", amConfig->pid);
        return 0;
    }
    slog(0, SLOG_INFO, "preparing antman...");
    slog(0, SLOG_INFO, "\t- directory to watch: %s", amConfig->watchDir);
    slog(0, SLOG_INFO, "\t- changing working directory to: %s", amConfig->workingDir);
    slog(0, SLOG_INFO, "\t- redirecting antman log to file: %s/%s.log", amConfig->workingDir, logName);
    return startDaemon(amConfig);
}

/*
    main is the antman entry point
*/
int main(int argc, char *argv[])
{

    // set up the long flags
    static ko_longopt_t longopts[] = {
        {"start", ko_no_argument, 301},
        {"stop", ko_no_argument, 302},
        {"setWatchDir", ko_required_argument, 303},
        {"getPID", ko_no_argument, 304},
        {0, 0, 0}};

    // set up the job list
    int start = 0, stop = 0, getPID = 0;
    char *watchDir = "";

    // get the CLI info
    ketopt_t opt = KETOPT_INIT;
    int c;
    while ((c = ketopt(&opt, argc, argv, 1, "hvu:", longopts)) >= 0)
    {
        if (c == 'h')
        {
            printUsage();
            return 0;
        }
        else if (c == 'v')
        {
            printf("%s\n", VERSION);
            return 0;
        }
        else if (c == 301)
            start = 1;
        else if (c == 302)
            stop = 1;
        else if (c == 303)
            opt.arg ? (watchDir = opt.arg) : (watchDir = AM_DEFAULT_WATCH_DIR);
        else if (c == 304)
        {
            getPID = 1;
        }
        else if (c == 'u')
            printf("unused flag:  -u %s\n", opt.arg);
        else if (c == '?')
        {
            fprintf(stderr, "unknown flag: -%c\n\n", opt.opt ? opt.opt : ':');
            printUsage();
            return 1;
        }
        else if (c == ':')
        {
            fprintf(stderr, "missing arg with flag: -%c\n\n", opt.opt ? opt.opt : ':');
            printUsage();
            return 1;
        }
    }

    // check we have a job to do, otherwise print the help screen and exit
    if (start + stop + getPID == 0 && (watchDir[0] == '\0'))
    {
        fprintf(stderr, "nothing to do: no flags set\n\n");
        printUsage();
        return 1;
    }

    // check the config exists and is accessible
    if (access(AM_DEFAULT_CONFIG, F_OK) == -1)
    {
        // config doesn't exist, so create one
        config_t *tmp = initConfig();
        if (writeConfig(tmp, AM_DEFAULT_CONFIG) != 0)
        {
            fprintf(stderr, "error: failed to create a config file - check permissions\n\n");
            exit(1);
        }
        destroyConfig(tmp);
    }
    if (access(AM_DEFAULT_CONFIG, W_OK) == -1)
    {
        fprintf(stderr, "error: failed to write to config file - check permissions\n\n");
        exit(1);
    }
    config_t *amConfig = initConfig();
    if (loadConfig(amConfig, AM_DEFAULT_CONFIG) != 0)
    {
        destroyConfig(amConfig);
        fprintf(stderr, "error: failed to load config file\n\n");
        exit(1);
    }

    // get the PID of the antman daemon (returns -1 if no daemon is running)
    int daemonPID = checkPID(amConfig);

    // report PID if requested and exit
    if (getPID == 1)
    {
        printf("%d\n", daemonPID);
        destroyConfig(amConfig);
        return 0;
    }

    // set up the log
    char logName[128];
    snprintf(logName, sizeof logName, "%s%s%s", AM_PROG_NAME, "-", getTimestamp());
    slog_init(logName, "log/slog.cfg", 4, 1);
    slog(0, SLOG_INFO, "starting antman (version: %s)", VERSION);
    slog(0, SLOG_INFO, "\t- using config: %s", AM_DEFAULT_CONFIG);

    // handle any stop request
    if (stop == 1)
    {
        if (stopAntman(amConfig) != 0)
            exit(1);
    }

    // handle any setWatchDir request
    if (watchDir[0] != '\0')
    {

        // if the daemon is already running, stop it, update the watch dir, then start the daemon again
        if (amConfig->running)
        {
            if (stopAntman(amConfig) != 0)
                exit(1);
            if (setAntman(amConfig, watchDir) != 0)
                exit(1);
            if (startAntman(amConfig, logName) != 0)
            {
                return 1;
            }
        }
        else
        {
            if (setAntman(amConfig, watchDir) != 0)
                exit(1);
        }
    }

    // handle any start request
    if (start == 1)
    {
        startAntman(amConfig, logName);
    }

    destroyConfig(amConfig);
    slog(0, SLOG_INFO, "donzo.");
    return 0;
}
