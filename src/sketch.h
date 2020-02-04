/*****************************************************************************
 * Package sketch.
 * 
 * TODO: make this function more generic - add in different sketching options
 * e.g., bloom, kmv etc.
 */
#ifndef SKETCH_H
#define SKETCH_H

#include <stdint.h>

#include "bloomfilter.h"

/*****************************************************************************
 * function prototypes
 */
int sketchSequence(const char *str, int len, int k, int sketchSize, bloomfilter_t *bf, uint64_t *sketchPtr);

#endif