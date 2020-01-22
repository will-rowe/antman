/*****************************************************************************
 * Package bigsi is a WIP library for Bit Sliced Genomic Signature Indexing.
 * 
 * To begin, add bloom filters to the BIGSI data structure with bigsAdd().
 * These bloom filters are provided via a map, where each key is a sequence
 * identifier and the value is a sequence bloom filter. bigsAdd() will check
 * the bloom filters and the corresponding sequence identifier, e.g. for
 * duplicate entries or incompatible settings, before assigning colours and
 * adding them to some temporary storage (bvArray and id2colour)
 * 
 * Once the desired number of bloom filters has been added (bigsAdd() can be
 * called multiple times), the BIGSI data structure can be indexed by calling
 * the bigsIndex() function.
 * 
 * bigsIndex() will iterate over all the bloom filters in the data structure,
 * creating a new bit vector for each bit index across all sequence IDs.
 * 
 * Once bigsIndex() has been run, it cannot be run again (at present) and all
 * input bloom filters will be deleted to save space.
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
    bigsi_t is the data structure for the bigsi index
*/
typedef struct bigsi
{
    bitvector_t **bvArray; // array of sequence bit vectors, indexed by their colour -> used prior to indexing
    map_int_t id2colour;   // map of sequence ID to colour -> used prior to indexing
    char **colourArray;    // array of sequence IDs, indexed by their colour
    bitvector_t **index;   // array of bigsi bit vectors, indexed by bit index of input bloom filters
    int numBits;           // number of bits in each input bloom filter
    int numBytes;          // the number of bytes needed to store numBits
    int numHashes;         // number of hash functions used to generate input bloom filter
    int colourIterator;    // used to assign colours to input bloom filters
} bigsi_t;

/*
    function prototypes
*/
bigsi_t *bigsInit(int numBits, int numHashes);
int bigsAdd(bigsi_t *bigsi, map_bloomfilter_t id2bf, int numEntries);
int bigsIndex(bigsi_t *bigsi);
int bigsQuery(bigsi_t *bigsi, const void *buffer, int len, bitvector_t *result);
int bigsDestroy(bigsi_t *bigsi);
int bigsDump(bigsi_t *bigsi, const char *filepath);
bigsi_t *bigsLoad(const char *filepath);

#endif