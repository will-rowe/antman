
/*****************************************************************************
 * subcommands contains subcommands and necessary helper functions.
 */
#ifndef SUBCMDS_H
#define SUBCMDS_H

#include <stdbool.h>

#include "../src/config.h"

/*****************************************************************************
 * function prototypes
 */

// subcommands
int sketch(config_t *config, char *filePath);
int info(config_t *config, bool pidOnly);
int stop(config_t *amConfig);
int shrink(config_t *amConfig);

// helpers
int checkPID(config_t *amConfig);

#endif