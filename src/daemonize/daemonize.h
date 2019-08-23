#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include "../config/config.h"

/*
    function declarations
*/
int startDaemon(char* daemonName, char* wdir, Config* amConfig);
int daemonize(char* name, char* path, char* outfile, char* errfile, char* infile);
void *launchWatcher(void* amConfig);
void *miscWorker(void *threadid);

#endif