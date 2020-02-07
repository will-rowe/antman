#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/3rd-party/slog.h"

#include "subcommands.h"
#include "../src/config.h"
#include "../src/daemonize.h"
#include "../src/errors.h"
#include "../src/watcher.h"
#include "../src/workerpool.h"

volatile sig_atomic_t SHUTDOWN_SIGNAL = 0;

/*****************************************************************************
 * stop will stop the daemon. It issues a SIGTERM and updates the config.
 *
 * note:
 *      TODO: instead of using SIGTERM, use waitpid and then decide to
 *              use a SIGKILL before harvesting zombies
 */
int stop(config_t *config)
{
    // check if the daemon is running
    if (checkPID(config) < 0)
    {
        fprintf(stderr, "no daemon currently running\n");
        return -1;
    }

    // kill
    if (kill(config->pid, SIGTERM) != 0)
    {
        fprintf(stderr, "could not kill the daemon process on PID %d\n", config->pid);
        return -1;
    }

    // update
    config->pid = -1;
    if (configWrite(config, config->filename) != 0)
    {
        fprintf(stderr, "could not update config file after stopping daemon\n");
        return -1;
    }
    return 0;
}

/*****************************************************************************
 * shrink converts the current program to a daemon process, launches some
 * threads, sets up a callback function and starts directory watching.
 */
int shrink(config_t *config)
{
    // TODO: error handling is pretty rubbish throughout this func, start working on consistency
    int err = 0;

    // divert log to file
    slog(0, SLOG_LIVE, "\t- redirected log to file: %s", config->currentLogFile);
    SlogConfig slgCfg;
    slog_config_get(&slgCfg);
    slgCfg.nToFile = 1;
    slgCfg.nFileStamp = 0;
    slgCfg.nTdSafe = 1;
    slog_config_set(&slgCfg);

    // try daemonising the program
    int res;
    if ((res = daemonize(PROG_NAME, NULL, NULL, NULL, NULL)) != 0)
    {
        slog(0, SLOG_ERROR, "could not start the daemon");
        return 1;
    }

    // set up the signal catcher
    catchSigterm();

    // log some progress
    slog(0, SLOG_INFO, "checking the daemon...");
    pid_t pid = getpid();
    slog(0, SLOG_LIVE, "\t- daemon pid: %d", pid);

    // update the config with the PID
    // TODO: this should probably be done in a lock file instead...
    config->pid = pid;
    if (configWrite(config, config->filename) != 0)
    {
        slog(0, SLOG_ERROR, "failed to update config file");
        return -1;
    }

    // create a threadpool
    slog(0, SLOG_INFO, "creating threadpool...");
    tpool_t *tp;
    tp = tpool_create(config->numThreads);
    slog(0, SLOG_LIVE, "\t- created threadpool of %d threads", config->numThreads);

    // create the watcher
    slog(0, SLOG_INFO, "creating the watcher...");
    watcherArgs_t *wargs = wargsInit(tp, config);
    if (!wargs)
    {
        slog(0, SLOG_ERROR, "failed to create arguments for the watcher");
        return -1;
    }
    if (FSW_OK != fsw_init_library())
    {
        slog(0, SLOG_ERROR, "fswatch cannot be initialised");
        slog(0, SLOG_LIVE, "\t- %s", fsw_last_error());
        wargsDestroy(wargs);
        return -1;
    }
    slog(0, SLOG_INFO, "\t- initialised fswatch");
    const FSW_HANDLE handle = fsw_init_session(fsevents_monitor_type);

    // add the path(s) for the watcher to watch
    if (FSW_OK != fsw_add_path(handle, config->watchDir))
    {
        slog(0, SLOG_ERROR, "could not add a path for fswatch: %s", config->watchDir);
        wargsDestroy(wargs);
        return 1;
    }
    slog(0, SLOG_LIVE, "\t- added watch directory");

    // set the watcher callback function
    if (FSW_OK != fsw_set_callback(handle, createCallback, wargs))
    {
        slog(0, SLOG_ERROR, "could not set the callback function for fswatch");
        wargsDestroy(wargs);
        return 1;
    }
    slog(0, SLOG_LIVE, "\t- set the callbacks");

    // start the watcher on a new thread
    pthread_t start_thread;
    if (pthread_create(&start_thread, NULL, startWatching, (void *)&handle))
    {
        slog(0, SLOG_ERROR, "could not start the watcher thread");
        wargsDestroy(wargs);
        return 1;
    }
    slog(0, SLOG_LIVE, "\t- started the watcher on a new thread");
    slog(0, SLOG_INFO, "waiting for sequence data...");

    // run daemon until a stop signal is received
    while (!SHUTDOWN_SIGNAL)
    {
        pause();
    }

    // stop the directory watcher
    if (FSW_OK != fsw_stop_monitor(handle))
    {
        slog(0, SLOG_ERROR, "error stopping the directory watcher");
        wargsDestroy(wargs);
        return 1;
    }
    slog(0, SLOG_LIVE, "\t- stopped the directory watcher");

    // TODO: sleep is needed here to have a graceful shutdown - work out a better way
    sleep(5);
    if (FSW_OK != fsw_destroy_session(handle))
    {
        slog(0, SLOG_ERROR, "error destroying the fswatch session");
        wargsDestroy(wargs);
        return 1;
    }
    slog(0, SLOG_LIVE, "\t- destroyed the fswatch session");

    // wait for the directory watcher thread to finish
    if (pthread_join(start_thread, NULL))
    {
        slog(0, SLOG_ERROR, "error rejoining directory watcher thread");
        wargsDestroy(wargs);
        return 1;
    }
    slog(0, SLOG_LIVE, "\t- directory watcher thread terminated");

    // wait on any active threads in the threadpool
    slog(0, SLOG_LIVE, "\t- waiting on threadpool");
    tpool_wait(tp);
    slog(0, SLOG_LIVE, "\t- threadpool idle");

    // destroy the watcher arguments, config and threadpool
    if ((err = wargsDestroy(wargs)) != 0)
    {
        slog(0, SLOG_ERROR, "%s", printError(err));
        return -1;
    }

    slog(0, SLOG_INFO, "donzo.");
    return 0;
}

// sigTermHandler is called in the event of a SIGTERM signal
void sigTermHandler(int signum)
{
    slog(0, SLOG_INFO, "sigterm (%u) received, shutting down the daemon...", signum);
    SHUTDOWN_SIGNAL = 1;
}

// catchSigterm is used to exit the daemon when `antman stop` is called
void catchSigterm(watcherArgs_t *wargs)
{
    static struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = sigTermHandler;
    sigaction(SIGTERM, &action, NULL);
}