/*****************************************************************************
 * Package sequence.
 */
#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "bloomfilter.h"

/*****************************************************************************
 * function prototypes
 */
void processRef(char *filepath, bloomfilter_t *bf, int kSize, int sketchSize);
void processFastq(void *arg);

#endif