#ifndef BIGSI_H
#define BIGSI_H

#include "bloom.h"
#include "map.h"

/*
    create the type map_uchar_t for storing bit vectors
*/
typedef map_t(unsigned char *) map_uchar_t;

/*
    bigsi_t is the data structure for the bigsi index
*/
typedef struct bigsi
{
    map_int_t id2colour;    // map of sequence ID to colour
    char **colourArray;     // array of sequence IDs, indexed by their colour
    map_uchar_t bitvectors; // map of each sequence ID to sequence bit vector
    int numBits;            // number of bits in each bit vector
    int numBytes;           // the number of bytes needed to store numBits
    int numHashes;          // number of hash functions used to generate bit vectors
    int colourIterator;     // used to assign colours to sequence IDs
    int numColourBytes;     // number of bytes needed to store colours
    unsigned char **index;  // the actual index
} bigsi_t;

/*
    function prototypes
*/
bigsi_t *initBIGSI(int numBits, int numHashes);
int insertBIGSI(bigsi_t *bigsi, map_uchar_t id2bv, int numEntries);
int indexBIGSI(bigsi_t *bigsi);
int queryBIGSI(bigsi_t *bigsi, char *kmer, int kSize, unsigned char *result);
void destroyBIGSI(bigsi_t *bigsi);

#endif