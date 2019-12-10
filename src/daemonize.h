#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include <libfswatch/c/libfswatch.h>

#include "config.h"

/*
    function prototypes
*/
void sigTermHandler(int signum);
void catchSigterm();
void *startWatching(void *param);
int startDaemon(config_t *amConfig);
int daemonize(char *name, char *path, char *outfile, char *errfile, char *infile);

#endif