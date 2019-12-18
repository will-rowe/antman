#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "ketopt.h"
#include "config.h"
#include "daemonize.h"
#include "slog.h"

/*
   printUsage prints the usage info for antman
*/
void printUsage(void)
{
    printf("usage:\tantman [flags]\n\n"
           "flags:\n"
           "\t --setWatchDir <=path>                 \t set the watch directory (default: %s)\n"
           "\t --setWhiteList <=path/filename>      \t set the white list\n"
           "\t --setLog <=path/filename>            \t set the log file\n"
           "\t --start                              \t start the antman daemon\n"
           "\t --stop                               \t stop the antman daemon\n"
           "\t --getPID                             \t prints PID of the antman daemon and exits\n"
           "\n"
           "\t -h                                   \t prints this help and exits\n"
           "\t -v                                   \t prints version number and exits\n",
           DEFAULT_WATCH_DIR);
}

/*
    checkPID returns the PID of the running daemon
    - checks if the antman daemon is running
    - then checks if the PID is correct (-2 if the PID isn't found)
    - returns the PID of the antman daemon (-1 if no daemon is running)

    TODO: check if PID name is antman:
        something like #ps -p $(./src/antman --getPID) -o comm=
*/
int checkPID(config_t *amConfig)
{
    if (amConfig->pid >= 0)
    {
        if (kill(amConfig->pid, 0) != 0)
        {
            fprintf(stderr, "\nerror: the registered antman pid is not running\n\n");
            return (-2);
        }
        return amConfig->pid;
    }
    else
    {
        return -1;
    }
}

/*
    stopAntman stops the daemon
    - issues SIGTERM to antman daemon
    - updates the config

    TODO: instead of using SIGTERM, use waitpid and then decide to use a SIGKILL before harvesting zombies
*/
int stopAntman(config_t *amConfig)
{
    if (kill(amConfig->pid, SIGTERM) != 0)
    {
        fprintf(stderr, "\nerror: could not kill the daemon process running on PID %d\n\n", amConfig->pid);
        return 1;
    }
    amConfig->pid = -1;
    if (writeConfig(amConfig, amConfig->filename) != 0)
    {
        fprintf(stderr, "\nerror: could not update the config file after stopping daemon\n\n");
        return 1;
    }
    return 0;
}

/*
    setWatchDir sets the watch directory
    - checks the directory exists
    - checks the directory is accessible
*/
int setWatchDir(config_t *amConfig, char *dirName)
{
    DIR *dir = opendir(dirName);
    if (dir)
    {
        closedir(dir);
        amConfig->watch_directory = dirName;
        return 0;
    }
    else if (ENOENT == errno)
    {
        fprintf(stderr, "\nerror: specified directory does not exist: %s\n\n", dirName);
        return 1;
    }
    else
    {
        fprintf(stderr, "\nerror: can't access the specified directory: %s\n\n", dirName);
        return 1;
    }
}

/*
    setWhiteList sets the white list
    - checks the file exists
    - checks the file is accessible
    - checks it is a multifasta
*/
int setWhiteList(config_t *amConfig, char *fileName)
{

    return 0;
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
        {"setWatchDir", ko_optional_argument, 303},
        {"setWhiteList", ko_optional_argument, 304},
        {"setLog", ko_optional_argument, 305},
        {"getPID", ko_no_argument, 306},
        {0, 0, 0}};

    // set up the job list
    int start = 0, stop = 0, getPID = 0;
    char *watchDir = "";
    char *whiteList = "";
    char *logFile = "";

    // get a default log name
    time_t timer;
    time(&timer);
    struct tm *tm_info;
    tm_info = localtime(&timer);
    static char defaultLog[28];
    strftime(defaultLog, 28, "./antman-%Y-%m-%d-%H%M.log", tm_info);

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
            printf("%s\n", AM_VERSION);
            return 0;
        }
        else if (c == 301)
            start = 1;
        else if (c == 302)
            stop = 1;
        else if (c == 303)
            opt.arg ? (watchDir = opt.arg) : (watchDir = DEFAULT_WATCH_DIR);
        else if (c == 304)
        {
            if (opt.arg != NULL)
            {
                whiteList = opt.arg;
            }
        }
        else if (c == 305)
            opt.arg ? (logFile = opt.arg) : (logFile = defaultLog);
        else if (c == 306)
            getPID = 1;
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
    if (start + stop + getPID == 0 && (watchDir[0] == '\0') && (logFile[0] == '\0') && (whiteList[0] == '\0'))
    {
        fprintf(stderr, "nothing to do: no flags set\n\n");
        printUsage();
        return 1;
    }

    // check the config exists and is accessible
    if (access(CONFIG_LOCATION, F_OK) == -1)
    {
        // config doesn't exist, so create one
        config_t *tmp = initConfig();
        if (writeConfig(tmp, CONFIG_LOCATION) != 0)
        {
            fprintf(stderr, "\nerror: failed to create a config file\n\n");
            return 1;
        }
        destroyConfig(tmp);
    }
    if (access(CONFIG_LOCATION, W_OK) == -1)
    {
        fprintf(stderr, "\nerror: failed to write to config file (check permissions)\n\n");
        return 1;
    }

    // load the config
    config_t *amConfig = initConfig();
    if (loadConfig(amConfig, CONFIG_LOCATION) != 0)
    {
        destroyConfig(amConfig);
        fprintf(stderr, "\nerror: failed to load config file\n\n");
        return 1;
    }

    // get the PID of the running antman daemon (returns -1 if no daemon is running)
    int daemonPID = checkPID(amConfig);
    if (daemonPID == -2)
    {
        destroyConfig(amConfig);
        return 1;
    }

    //
    // READY TO START HANDLING REQUESTS
    //

    // handle any --getPID request (and then exit)
    if (getPID == 1)
    {
        printf("%d\n", daemonPID);
        destroyConfig(amConfig);
        return 0;
    }

    // handle any --stop request
    if (stop == 1)
    {
        if (daemonPID == -1)
        {
            fprintf(stderr, "\nerror: no daemon running, nothing to stop\n\n");
            destroyConfig(amConfig);
            return 1;
        }
        if (stopAntman(amConfig) != 0)
            destroyConfig(amConfig);
        return 1;
        fprintf(stdout, "\nsuccess: stopped the daemon process running on PID %d\n", daemonPID);
        fprintf(stdout, "\t- view full log at: %s\n\n", amConfig->current_log_file);
    }

    // handle any --setWatchDir, --setWhiteList or --setLog requests
    if (watchDir[0] != '\0' || whiteList[0] != '\0' || logFile[0] != '\0')
    {

        // if the daemon is already running, stop it first (if we didn't just stop it with --stop)
        if (daemonPID >= 0 && stop == 0)
        {
            if (stopAntman(amConfig) != 0)
            {
                destroyConfig(amConfig);
                return 1;
            }
        }

        // set the watch directory if requested
        if (watchDir[0] != '\0')
        {
            if (setWatchDir(amConfig, watchDir) != 0)
            {
                destroyConfig(amConfig);
                return 1;
            }
            fprintf(stdout, "set the watch directory to %s\n", amConfig->watch_directory);
        }

        // set the whitelist if requested
        if (whiteList[0] != '\0')
        {
            if (setWhiteList(amConfig, watchDir) != 0)
            {
                destroyConfig(amConfig);
                return 1;
            }
            fprintf(stdout, "set the white list to %s\n", amConfig->watch_directory);
        }

        // set the log if requested
        if (logFile[0] != '\0')
        {
            amConfig->current_log_file = logFile;
            fprintf(stdout, "set the log file to %s\n", amConfig->current_log_file);
        }

        // update the config
        if (writeConfig(amConfig, amConfig->filename) != 0)
        {
            fprintf(stderr, "\nerror: could not update the config file\n\n");
            destroyConfig(amConfig);
            return 1;
        }

        // restart the daemon if we stopped it (--stop wasn't requested)
        if (daemonPID >= 0 && stop == 0)
        {
            fprintf(stdout, "\t- restarting the antman daemon now\n\n");
            start = 1;
        }
    }

    // handle any --start request
    if (start == 1)
    {

        // check the daemon is not already running
        if (amConfig->pid != -1)
        {
            fprintf(stderr, "\nerror: the daemon is already running on PID %d\n\n", amConfig->pid);
            destroyConfig(amConfig);
            return 1;
        }

        // make sure there is a log - create the default if needed
        if (amConfig->current_log_file[0] == '\0')
        {
            amConfig->current_log_file = defaultLog;
        }

        // start logging
        slog_init(amConfig->current_log_file, "log/slog.cfg", 4, 1);
        slog(0, SLOG_INFO, "starting antman log (version: %s)", AM_VERSION);
        slog(0, SLOG_INFO, "\t- using config: %s", CONFIG_LOCATION);
        slog(0, SLOG_INFO, "preparing antman...");
        slog(0, SLOG_INFO, "\t- directory to watch: %s", amConfig->watch_directory);

        // start the daemon
        if (startDaemon(amConfig) != 0)
        {
            destroyConfig(amConfig);
            return 1;
        }
        slog(0, SLOG_INFO, "donzo.");
    }
    destroyConfig(amConfig);
    return 0;
}
