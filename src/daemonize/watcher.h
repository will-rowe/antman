#ifndef WATCHER_H
#define WATCHER_H

#include <libfswatch/c/libfswatch.h>

/*
    function declarations
*/
char* getExt(const char *filename);
void watcherCallback(fsw_cevent const * const events, const unsigned int event_num, void * data);
int setupWatcher(char* watchPath);

#endif