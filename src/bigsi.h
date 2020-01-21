/*****************************************************************************
 * Package bigsi is a WIP library for Bit Sliced Genomic Signature Indexing.
 */
#ifndef BIGSI_H
#define BIGSI_H

#include "bloomfilter.h"
#include "bitvector.h"
#include "map.h"

/*
    create the type map_bloomfilter_t for storing bloomfilters
*/
typedef map_t(bloomfilter_t) map_bloomfilter_t;

/*
    create the type map_bitvector_t for storing bitvector
*/
typedef map_t(bitvector_t) map_bitvector_t;

/*
    bigsi_t is the data structure for the bigsi index
*/
typedef struct bigsi
{
    map_int_t id2colour;   // map of sequence ID to colour
    char **colourArray;    // array of sequence IDs, indexed by their colour
    bitvector_t **bvArray; // array of sequence bit vectors, indexed by their colour
    int numBits;           // number of bits in each bit vector
    int numBytes;          // the number of bytes needed to store numBits
    int numHashes;         // number of hash functions used to generate bit vectors
    int colourIterator;    // used to assign colours to sequence IDs
    bitvector_t **index;   // the actual index
} bigsi_t;

/*
    function prototypes
*/
bigsi_t *bigsInit(int numBits, int numHashes);
int bigsAdd(bigsi_t *bigsi, map_bloomfilter_t id2bf, int numEntries);
int bigsIndex(bigsi_t *bigsi);
int bigsQuery(bigsi_t *bigsi, const void *buffer, int len, bitvector_t *result);
int bigsDestroy(bigsi_t *bigsi);

#endif