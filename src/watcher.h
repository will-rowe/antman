#ifndef WATCHER_H
#define WATCHER_H

#include <libfswatch/c/libfswatch.h>
#include "bloom.h"
#include "workerpool.h"

// watcherArgs_t
typedef struct watcherArgs {
    tpool_t* workerPool;
    struct bloom* bloomFilter;
    char filepath[50];
    int k_size;
    int sketch_size;
    double fp_rate;
} watcherArgs_t;

/*
    function prototypes
*/
char* getExt(const char *filename);
void watcherCallback(fsw_cevent const * const events, const unsigned int event_num, void* args);

#endif