#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include <libfswatch/c/libfswatch.h>

#include "config.h"
#include "watcher.h"

/*
    function prototypes
*/
void sigTermHandler(int signum);
void catchSigterm();
void *startWatching(void *param);
int startDaemon(config_t *amConfig, watcherArgs_t *wargs);
int daemonize(char *name, char *path, char *outfile, char *errfile, char *infile);

#endif