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
   getLogName creates a default name for the log file
*/
char *getLogName()
{
    time_t timer;
    static char buffer[28];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 28, "./antman-%Y-%m-%d-%H%M.log", tm_info);
    puts(buffer);
    return buffer;
}

/*
   printUsage prints the usage info for antman
*/
void printUsage(void)
{
    printf("usage:\tantman [flags]\n\n"
           "flags:\n"
           "\t --start                       \t start the antman daemon\n"
           "\t --stop                        \t stop the antman daemon\n"
           "\t --setWatchDir <path>          \t set the watch directory (default: ./antman-YYYY-MM-DD-HHMM.log)\n"
           "\t --setLog <path/filename>      \t set log file (default: %s)\n"
           "\t --getPID                      \t return the PID of the antman daemon\n"
           "\n"
           "\t -h                            \t prints this help and exits\n"
           "\t -v                            \t prints version number and exits\n",
           AM_DEFAULT_WATCH_DIR);
}

/*
  checkPID
   - checks if the antman daemon is running
   - then checks if the PID is correct (-2 if the PID isn't found)
   - returns the PID of the antman daemon (-1 if no daemon is running)
*/
int checkPID(config_t *amConfig)
{

    // if running, check the PID
    if (amConfig->pid >= 0)
    {
        // check it exists
        if (kill(amConfig->pid, 0) != 0)
        {
            fprintf(stderr, "error: the registered antman pid is not running\n\n");
            return(-2);
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
    if (kill(amConfig->pid, SIGTERM) != 0)
    {
        fprintf(stderr, "error: could not kill the daemon process running on PID %d\n\n", amConfig->pid);
        return 1;
    }

    //TODO: instead of the above, use waitpid, then decide to use a SIGKILL and then harvest zombies

    fprintf(stdout, "success: stopped the daemon process running on PID %d\n", amConfig->pid);
    fprintf(stdout, "\t- view full log at: %s\n\n", amConfig->logFile);

    // update the config
    amConfig->pid = -1;
    if (writeConfig(amConfig, amConfig->configFile) != 0)
    {
        fprintf(stderr, "error: could not update the config file after stopping daemon\n\n");
        return 1;
    }
    return 0;
}

/*
   setAntman is used to set the watch directory
*/
int setAntman(config_t *amConfig, char *dirName)
{

    // check directory exists and is accessible
    DIR *dir = opendir(dirName);
    if (dir)
    {
        closedir(dir);

        // update the config and re-write the file
        amConfig->watchDir = dirName;
        if (writeConfig(amConfig, amConfig->configFile) != 0)
        {
            fprintf(stderr, "error: failed to update config file with new watch directory\n\n");
            return 1;
        }
    }
    else if (ENOENT == errno)
    {
        fprintf(stderr, "error: specified directory does not exist: %s\n\n", dirName);
        return 1;
    }
    else
    {
        fprintf(stderr, "error: can't access the specified directory: %s\n\n", dirName);
        return 1;
    }
    fprintf(stdout, "set the watch directory to: %s\n\n", amConfig->watchDir);
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
        {"setLog", ko_optional_argument, 304},
        {"getPID", ko_no_argument, 305},
        {0, 0, 0}};

    // set up the job list
    int start = 0, stop = 0, getPID = 0;
    char *watchDir = "";
    char *logFile = "";

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
        else if (c == 301) start = 1;
        else if (c == 302) stop = 1;
        else if (c == 303) opt.arg ? (watchDir = opt.arg) : (watchDir = AM_DEFAULT_WATCH_DIR);
        else if (c == 304) opt.arg ? (logFile = opt.arg) : (logFile = getLogName());
        
        else if (c == 305) getPID = 1;
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
            return 1;
        }
        destroyConfig(tmp);
    }
    if (access(AM_DEFAULT_CONFIG, W_OK) == -1)
    {
        fprintf(stderr, "error: failed to write to config file - check permissions\n\n");
        return 1;
    }
    config_t *amConfig = initConfig();
    if (loadConfig(amConfig, AM_DEFAULT_CONFIG) != 0)
    {
        destroyConfig(amConfig);
        fprintf(stderr, "error: failed to load config file\n\n");
        return 1;
    }

    // get the PID of the running antman daemon (returns -1 if no daemon is running)
    int daemonPID = checkPID(amConfig);
    if (daemonPID == -2) 
    {
        destroyConfig(amConfig);
        return 1; 
    }

    // handle any --getPID request
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
            fprintf(stderr, "error: no daemon running, nothing to stop\n\n");
            return 1;
        }
        if (stopAntman(amConfig) != 0)
            return 1;
    }

    // handle any --setWatchDir request
    if (watchDir[0] != '\0')
    {

        // if the daemon is already running, stop it first
        if (amConfig->pid >= 0)
        {
            if (stopAntman(amConfig) != 0)
                destroyConfig(amConfig);
                return 1;
        }
        if (setAntman(amConfig, watchDir) != 0)
            destroyConfig(amConfig);
            return 1;
    }

    // handle any --start request (or issue a restart after --setWatchDir)
    if (start == 1 || watchDir[0] != '\0')
    {
        if (amConfig->pid != -1)
        {
            fprintf(stderr, "error: the daemon is already running on PID %d\n\n", amConfig->pid);
            return 1;
        }

        // set up the log (use default log if no file provided)
        if (logFile[0] == '\0') {
            logFile = getLogName();
        }
        amConfig->logFile = logFile;
        slog_init(logFile, "log/slog.cfg", 4, 1);
        slog(0, SLOG_INFO, "starting antman log (version: %s)", AM_VERSION);
        slog(0, SLOG_INFO, "\t- using config: %s", AM_DEFAULT_CONFIG);
        slog(0, SLOG_INFO, "preparing antman...");
        slog(0, SLOG_INFO, "\t- directory to watch: %s", amConfig->watchDir);

        // start the daemon
        if (startDaemon(amConfig) != 0) {
            return 1;
        }
        slog(0, SLOG_INFO, "donzo.");
    }

    destroyConfig(amConfig);
    return 0;
}
