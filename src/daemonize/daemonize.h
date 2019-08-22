#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include "../config/config.h"

/*
    function declarations
*/
void *launchWatcher(void* ddconfig);
void *miscWorker(void *threadid);
void startDaemon(char* daemonName, char* wdir, Config* ddconfig);
int daemonize(char* name, char* path, char* outfile, char* errfile, char* infile);

#endif