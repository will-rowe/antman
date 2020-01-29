/*****************************************************************************
 * Package bitvector is a library to create, query and compare bit vectors.
 * 
 * The bit vectors use 0 based indexing. This is important to remember when
 * querying etc. For example: you can only flip bits 0-9 in a bit vector with
 * a capacity of 10.
 */
#ifndef BITVECTOR_H
#define BITVECTOR_H

#include <stdint.h>

/*****************************************************************************
 * bitvector_t is the main data structure
 */
typedef struct BitVector
{
    uint64_t capacity; // maximum number of bits the bit vector can hold
    uint64_t bufSize;  // the required number of bytes to fit the bit capacity
    uint64_t count;    // the current number of set bits in the bit vector
    uint8_t buffer[];  // the actual bit vector, stored as a flexible array member
} bitvector_t;

/*****************************************************************************
 * function prototypes
 */
bitvector_t *bvInit(uint64_t capacity);
bitvector_t *bvClone(const bitvector_t *bv);
uint64_t bvCapacity(const bitvector_t *bv);
uint64_t bvCount(const bitvector_t *bv);
int bvSet(bitvector_t *bv, uint64_t bit, uint8_t val);
int bvGet(bitvector_t *bv, uint64_t bit, uint8_t *result);
int bvClear(bitvector_t *bv);
int bvDestroy(bitvector_t *bv);
void bvPrint(bitvector_t *bv);
int bvBAND(const bitvector_t *bv1, const bitvector_t *bv2, bitvector_t *result);
int bvBOR(const bitvector_t *bv1, const bitvector_t *bv2, bitvector_t *result);
int bvXOR(const bitvector_t *bv1, const bitvector_t *bv2, bitvector_t *result);
int bvBANDupdate(bitvector_t *bv1, const bitvector_t *bv2);

#endif