/*****************************************************************************
 * Package bloomfilter is a library to create and query bloom filters.
 */
#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <stdint.h>
#include "bitvector.h"

/*****************************************************************************
 * bloom filter struct
 */
typedef struct BloomFilter
{
    uint64_t estCapacity;        // maximum number of entries expected
    double collisionProbability; // the collision probability
    uint64_t numHashes;          // the number of hashes
    bitvector_t *bitvector;      // the underlying bit vector
} bloomfilter_t;

/*****************************************************************************
 * function prototypes
 */
unsigned int getHashVal(const void *buffer, int len, int iterator, int modulo);
bloomfilter_t *bfInit(uint64_t estCapacity, double collisionProbability);
int bfAdd(bloomfilter_t *bf, const void *buffer, uint64_t len);
int bfQuery(bloomfilter_t *bf, const void *buffer, uint64_t len, uint8_t *result);
int bfDestroy(bloomfilter_t *bf);

#endif