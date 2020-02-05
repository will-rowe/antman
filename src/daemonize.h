/*****************************************************************************
 * Package daemonize is used to control the daemon.
 */
#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include <libfswatch/c/libfswatch.h>

/*****************************************************************************
 * function prototypes
 */
void *startWatching(void *param);
int daemonize(char *name, char *path, char *outfile, char *errfile, char *infile);
void createCallback(fsw_cevent const *const events, const unsigned int event_num, void *args);

#endif