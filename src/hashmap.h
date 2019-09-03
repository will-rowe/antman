// simple hash map which uses linear probing to keep track of what hash values are currently in a KMV sketch
#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>

#define HASHMAP_SIZE 256

typedef struct kmer { 
   uint64_t kmerHash;
} kmer;

/*
    function prototypes
*/
bool hmInsert(uint64_t kmerHash);
bool hmSearch(uint64_t kmerHash);
void hmDelete(uint64_t kmerHash);
void hmDestroy(void);

#endif