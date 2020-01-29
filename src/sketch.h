/*****************************************************************************
 * Package sketch.
 */
#ifndef SKETCH_H
#define SKETCH_H

#include <stdint.h>

#include "bloomfilter.h"

/*****************************************************************************
 * function prototypes
 */
void sketchSequence(const char *str, int len, int k, int sketchSize, bloomfilter_t *bf, uint64_t *sketchPtr);

#endif