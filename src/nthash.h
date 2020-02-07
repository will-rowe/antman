/*****************************************************************************
 * Package ntHash is a copy of ntHash by Hamid Mohamadi, ported to C with only
 * the rolling canonical multihash included.
 * 
 * Author: Hamid Mohamadi
 * Genome Sciences Centre,
 * British Columbia Cancer Agency
 * 
 * see: https://github.com/bcgsc/ntHash
 * 
 * usage:
 * 
    // init
    nthash_iterator_t *nt = ntInit(SEQ, SEQLEN, KSIZE, NUMHASH);
    if (nt == NULL)
        return ERR_init;

    // iterate over the hashes for each k-mer
    while (!nt->end)
    {
        for (int i = 0; i < NUMHASH; i++)
        {
            if (nt->hashVector[i] == 0)
                return ERR_multiHash;
        }
        ntIterate(nt);
    }

    // clean up
    if (ntDestroy(nt) != 0)
        return ERR_destroy;
 * 
 */
#ifndef NTHASH_H
#define NTHASH_H

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
 * nthash_iterator_t is the main data structure for iterating over hash values
 * for k-mers in a sequence.
 */
typedef struct ntHashIterator
{
    uint64_t *hashVector; // hash value(s) for the current k-mer in the iterator
    bool end;             // true indicates that the iterator has finished
    size_t seqIterator;   // position of current k-mer in the sequence
    uint64_t m_fhVal;     // forward-strand k-mer hash value
    uint64_t m_rhVal;     // reverse-complement k-mer hash value

    // private members:
    char *seq;              // sequence
    unsigned int numHashes; // number of hashes
    unsigned int kSize;     // k-mer size
    unsigned int numKmers;  // the number of k-mers in the sequence
} nthash_iterator_t;

/*****************************************************************************
 * function prototypes
 */
nthash_iterator_t *ntInit(char *seq, unsigned int seqLen, unsigned int kSize, unsigned int numHash);
void ntPrepare(nthash_iterator_t *nt);
void ntIterate(nthash_iterator_t *nt);
int ntDestroy(nthash_iterator_t *nt);

#endif