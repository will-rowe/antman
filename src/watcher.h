/*****************************************************************************
 * Package watcher is used to watch for new sequence data and spawn new 
 * sketches and DB queries.
 * 
 * TODO: this package will be subject to a lot of change as we determine
 * the feature set we want for the first antman release.
 */
#ifndef WATCHER_H
#define WATCHER_H

#include "config.h"
#include "workerpool.h"

/*****************************************************************************
 * watcherArgs_t is used to pass information from the CLI to fswatch
 * callbacks.
 */
typedef struct watcherArgs
{
    tpool_t *threadPool; // available threads to do the work
    config_t *config;    // the in-memory config, for accessing parameters
} watcherArgs_t;

/*****************************************************************************
 * watcherJob_t is used to used to send work from the watcher to a
 * threadpool.
 */
typedef struct watcherJob
{
    watcherArgs_t *wargs;
    char *filepath; // the filepath to the sequence that needs sketching
} watcherJob_t;

/*****************************************************************************
 * function prototypes
 */
watcherArgs_t *wargsInit(tpool_t *tp, config_t *config);
int wargsDestroy(watcherArgs_t *wargs);
int wjobCreate(watcherArgs_t *wargs, char *fp, watcherJob_t **resultPtr);
int wjobDestroy(watcherJob_t *wjob);

#endif