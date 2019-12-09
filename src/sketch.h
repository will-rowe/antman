#ifndef SKETCH_H
#define SKETCH_H

#include <stdint.h>

#include "bloom.h"

/*
    function prototypes
*/
void sketchSequence(const char *str, int len, int k, int sketchSize, struct bloom *bf, uint64_t *sketchPtr);

#endif