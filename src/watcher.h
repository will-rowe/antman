/*****************************************************************************
 * Package watcher is used to watch for new sequence data and spawn new 
 * sketches and DB queries.
 * 
 * TODO: this package will be subject to a lot of change as we determine
 * the feature set we want for the first antman release.
 */
#ifndef WATCHER_H
#define WATCHER_H

#include <libfswatch/c/libfswatch.h>

#include "bloomfilter.h"
#include "workerpool.h"

/*****************************************************************************
 * watcherArgs_t is used to pass information from the CLI
 */
typedef struct watcherArgs
{
    tpool_t *workerPool;
    bloomfilter_t *bloomFilter;
    char filepath[50];
    int k_size;
    int sketch_size;
    double fp_rate;
} watcherArgs_t;

/*****************************************************************************
 * function prototypes
 */
char *getExt(const char *filename);
void watcherCallback(fsw_cevent const *const events, const unsigned int event_num, void *args);

#endif