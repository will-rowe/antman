#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // contains fork(3), chdir(3), sysconf(3)
#include <signal.h>   //contains signal(3)
#include <sys/stat.h> // contains umask(3)

#include "bloom.h"
#include "daemonize.h"
#include "sequence.h"
#include "slog.h"
#include "workerpool.h"

// TODO: set these values by the cli
const int NUM_THREADS = 4;

// done controls when to stop the antman threads
volatile sig_atomic_t done = 0;

// sigTermHandler is called in the event of a SIGTERM signal
void sigTermHandler(int signum)
{
    slog(0, SLOG_INFO, "sigterm received, shutting down the antman daemon...");
    done = 1;
}

// catchSigterm is used to exit the daemon when `antman --stop` is called
void catchSigterm()
{
    static struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = sigTermHandler;
    sigaction(SIGTERM, &action, NULL);
}

// startWatching is used to start the directory watcher inside a thread
void *startWatching(void *param)
{
    FSW_HANDLE *handle = (FSW_HANDLE *)param;
    if (FSW_OK != fsw_start_monitor(*handle))
    {
        slog(0, SLOG_ERROR, "could not start fswatch monitor");
        exit(1);
    }
    return NULL;
}

// startDaemon converts the current program to a daemon process, launches some threads and starts directory watching
int startDaemon(config_t *amConfig, watcherArgs_t *wargs)
{

    // try daemonising the program
    slog(0, SLOG_LIVE, "\t- redirected antman log to file: %s", amConfig->current_log_file);
    int res;
    if ((res = daemonize(AM_PROG_NAME, "NULL", NULL, NULL, NULL)) != 0)
    {
        slog(0, SLOG_ERROR, "could not start the antman daemon");
        return 1;
    }

    // divert log to file
    SlogConfig slgCfg;
    slog_config_get(&slgCfg);
    slgCfg.nToFile = 1;
    slgCfg.nFileStamp = 0;
    slgCfg.nTdSafe = 1;
    slog_config_set(&slgCfg);

    // log some progress
    slog(0, SLOG_INFO, "checking the antman daemon...");
    pid_t pid = getpid();
    slog(0, SLOG_LIVE, "\t- daemon pid: %d", pid);

    // update the config with the PID
    // TODO: this should probably be done in a lock file instead...
    amConfig->pid = pid;
    if (writeConfig(amConfig, amConfig->filename) != 0)
    {
        slog(0, SLOG_ERROR, "failed to update config file");
        return 1;
    }

    // set up the signal catcher
    catchSigterm();

    // initialise fswatch
    slog(0, SLOG_INFO, "initialising fswatch...");
    if (FSW_OK != fsw_init_library())
    {
        slog(0, SLOG_ERROR, "fswatch cannot be initialised");
        slog(0, SLOG_LIVE, "\t- %s", fsw_last_error());
        return 1;
    }
    const FSW_HANDLE handle = fsw_init_session(fsevents_monitor_type);

    // add the path(s) for the watcher to watch
    if (FSW_OK != fsw_add_path(handle, amConfig->watch_directory))
    {
        slog(0, SLOG_ERROR, "could not add a path for libfswatch: %s", amConfig->watch_directory);
        return 1;
    }
    slog(0, SLOG_LIVE, "\t- added directory to the watch path: %s", amConfig->watch_directory);

    // launch the worker threads
    slog(0, SLOG_INFO, "creating workerpool...");
    tpool_t *wp;
    wp = tpool_create(NUM_THREADS);
    slog(0, SLOG_LIVE, "\t- created workerpool of %d threads", NUM_THREADS);
    wargs->workerPool = wp;

    // set the watcher callback function
    if (FSW_OK != fsw_set_callback(handle, watcherCallback, wargs))
    {
        slog(0, SLOG_ERROR, "could not set the callback function for libfswatch");
        return 1;
    }

    // start the watcher on a new thread
    pthread_t start_thread;
    if (pthread_create(&start_thread, NULL, startWatching, (void *)&handle))
    {
        slog(0, SLOG_ERROR, "could not start the watcher thread");
        return 1;
    }
    slog(0, SLOG_INFO, "antman is waiting for sequence data...");

    // run antman until a stop signal is received
    while (!done)
    {
        pause();
    }

    // stop the directory watcher
    slog(0, SLOG_LIVE, "\t- stopping the directory watcher");
    if (FSW_OK != fsw_stop_monitor(handle))
    {
        slog(0, SLOG_ERROR, "error stopping the directory watcher");
        return 1;
    }
    sleep(5);
    if (FSW_OK != fsw_destroy_session(handle))
    {
        slog(0, SLOG_ERROR, "error destroying the fswatch session");
        return 1;
    }

    // wait for the directory watcher thread to finish
    if (pthread_join(start_thread, NULL))
    {
        slog(0, SLOG_ERROR, "error joining directory watcher thread");
        return 1;
    }

    // wait on any active threads in the workerpool
    slog(0, SLOG_LIVE, "\t- stopping the sketching threads");
    tpool_wait(wp);

    // destroy the workerpool
    tpool_destroy(wp);

    return 0;
}

// daemonize is used to fork, detach, fork again, change permissions, change directory and then reopen streams
int daemonize(char *name, char *path, char *outfile, char *errfile, char *infile)
{
    if (!path)
    {
        path = "/";
    }
    if (!name)
    {
        name = "antman";
    }
    if (!infile)
    {
        infile = "/dev/null";
    }
    if (!outfile)
    {
        outfile = "/dev/null";
    }
    if (!errfile)
    {
        errfile = "/dev/null";
    }

    pid_t child;

    //fork, detach from process group leader
    if ((child = fork()) < 0)
    {
        //failed fork
        fprintf(stderr, "error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if (child > 0)
    {
        //parent
        exit(EXIT_SUCCESS);
    }
    if (setsid() < 0)
    {
        //failed to become session leader
        fprintf(stderr, "error: failed setsid\n");
        exit(EXIT_FAILURE);
    }

    //catch/ignore signals
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    //fork second time
    if ((child = fork()) < 0)
    {
        //failed fork
        fprintf(stderr, "error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if (child > 0)
    {
        //parent
        exit(EXIT_SUCCESS);
    }

    //new file permissions
    umask(0);

    //change to path directory
    chdir(path);

    //close all open file descriptors
    int fd;
    for (fd = sysconf(_SC_OPEN_MAX); fd > 0; --fd)
    {
        close(fd);
    }

    //reopen stdin, stdout, stderr
    stdin = fopen(infile, "r");    //fd=0
    stdout = fopen(outfile, "w+"); //fd=1
    stderr = fopen(errfile, "w+"); //fd=2

    return (0);
}