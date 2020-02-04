/*****************************************************************************
 * Package bloomfilter is a library to create and query bloom filters.
 * 
 * TODO: I have added in the numBits field and bfInitWithSize() func, these
 * need tests.
 */
#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <stdint.h>
#include "bitvector.h"

/*****************************************************************************
 * bloomfilter_t is the main data structure
 */
typedef struct BloomFilter
{
    uint64_t estCapacity;        // maximum number of entries expected
    double collisionProbability; // the collision probability
    uint64_t numBits;            // the number of bits
    uint64_t numHashes;          // the number of hashes
    bitvector_t *bitvector;      // the underlying bit vector
} bloomfilter_t;

/*****************************************************************************
 * function prototypes
 */
unsigned int getHashVal(const void *buffer, int len, int iterator, int modulo);
int bfCalc(uint64_t estCapacity, double collisionProbability, uint64_t *numBitsPtr, uint64_t *numHashesPtr);
bloomfilter_t *bfInit(uint64_t estCapacity, double collisionProbability);
bloomfilter_t *bfInitWithSize(uint64_t numBits, uint64_t numHashes);
int bfAdd(bloomfilter_t *bf, const void *buffer, uint64_t len);
int bfQuery(bloomfilter_t *bf, const void *buffer, uint64_t len, uint8_t *result);
int bfDestroy(bloomfilter_t *bf);

#endif