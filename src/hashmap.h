/*****************************************************************************
 * Package hashmap is a simple hash map that uses linear probing.
 * 
 * Currently this is only used to keep track of values in the KMV sketch
 * package.
 */
#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>
#include <stdint.h>

#define HASHMAP_SIZE 256

/*****************************************************************************
 * kmer_t is the hashed k-mer being kept in the KMV sketch
 */
typedef struct KMER
{
    uint64_t kmerHash;
} kmer_t;

/*****************************************************************************
 * function prototypes
 */
bool hmInsert(uint64_t kmerHash);
bool hmSearch(uint64_t kmerHash);
void hmDelete(uint64_t kmerHash);
void hmDestroy(void);

#endif