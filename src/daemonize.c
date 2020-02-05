#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // contains fork(3), chdir(3), sysconf(3)
#include <signal.h>   //contains signal(3)
#include <sys/stat.h> // contains umask(3)

#include "3rd-party/slog.h"

#include "daemonize.h"
#include "errors.h"
#include "helpers.h"
#include "sequence.h"
#include "watcher.h"

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

// daemonize is used to fork, detach, fork again, change permissions, change directory and then reopen streams
int daemonize(char *name, char *path, char *outfile, char *errfile, char *infile)
{
    if (!name)
    {
        name = PROG_NAME;
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

    //change to path directory if requested
    if (path)
    {
        if (chdir(path) != 0)
        {
            fprintf(stderr, "error: failed to change directory during daemonisation\n");
            exit(EXIT_FAILURE);
        }
    }

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

/*****************************************************************************
 * createCallback is the function that will be called when fswatch detects a
 * change in the watch directory.
 */
void createCallback(fsw_cevent const *const events, const unsigned int event_num, void *args)
{
    watcherArgs_t *wargs;
    wargs = (watcherArgs_t *)args;

    // set the flags from libfswatch that we want to check events against
    int fileCheckList = Created | IsFile;

    //NoOp = 0,                     /**< No event has occurred. */
    //PlatformSpecific = (1 << 0),  /**< Platform-specific placeholder for event type that cannot currently be mapped. */
    //Created = (1 << 1),           /**< An object was created. */
    //Updated = (1 << 2),           /**< An object was updated. */
    //Removed = (1 << 3),           /**< An object was removed. */
    //Renamed = (1 << 4),           /**< An object was renamed. */
    //OwnerModified = (1 << 5),     /**< The owner of an object was modified. */
    //AttributeModified = (1 << 6), /**< The attributes of an object were modified. */
    //MovedFrom = (1 << 7),         /**< An object was moved from this location. */
    //MovedTo = (1 << 8),           /**< An object was moved to this location. */
    //IsFile = (1 << 9),            /**< The object is a file. */
    //IsDir = (1 << 10),            /**< The object is a directory. */
    //IsSymLink = (1 << 11),        /**< The object is a symbolic link. */
    //Link = (1 << 12),             /**< The link count of an object has changed. */
    //Overflow = (1 << 13)          /**< The event queue has overflowed. */

    // loop over the events
    unsigned int i, j;
    for (i = 0; i < event_num; i++)
    {
        fsw_cevent const *e = &events[i];

        // check if the event concerns a filetype we are interested in
        char *ext = getExtension(e->path);
        if ((strcmp(ext, "fastq") == 0) || (strcmp(ext, "fq") == 0))
        {
            // combine the flags for the event into a bitmask
            unsigned int setFlags = 0;
            for (j = 0; j < e->flags_num; j++)
            {
                setFlags |= e->flags[j];
            }

            // use the bitmask to determine how to handle the event
            if ((setFlags & fileCheckList) == fileCheckList)
            {
                slog(0, SLOG_LIVE, "[watcher]\tfound a FASTQ file: %s", e->path);
            }
            else
            {
                if ((setFlags & 1 << 3) == 1 << 3)
                {
                    slog(0, SLOG_LIVE, "[watcher]\tignoring a deleted file");
                }
                else
                {
                    slog(0, SLOG_LIVE, "[watcher]\tignoring a file modification"); // TODO: is this catch right?
                }
                continue;
            }

            // call the watcherCreateJob function to check the file init a job
            watcherJob_t *job;
            int err = 0;
            if ((err = wjobCreate(wargs, events[i].path, &job)) != 0)
            {
                slog(0, SLOG_ERROR, "could not create watcher job");
                printError(err);
                return;
            }

            // add the job to the threadpool
            if (!tpool_add_work(wargs->threadPool, processFastq, job))
            {
                slog(0, SLOG_ERROR, "could not queue watcher job");
                return;
            }
        }
        else
        {
            slog(0, SLOG_LIVE, "[watcher]\tignoring a file (%s)", e->path);
        }
    }
}
