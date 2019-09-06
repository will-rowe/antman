#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "bloom.h"

/*
    function prototypes
*/
void processRef(char* filepath, struct bloom* bf, int kSize, int sketchSize);
void processFastq(void* arg);

#endif