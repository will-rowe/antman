#ifndef TEST_NTHASH
#define TEST_NTHASH

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "minunit.h"
#include "../src/nthash.h"

/*
Comparing this test to the example usage given in ntHash v2.1.0,
where the following hash values are expected for the 6 70-mers

seq:
GAGTGTCAAACATTCAGACAACAGCAGGGGTGCTCTGGAATCCTATGTGAGGAACAAACATTCAGGCCACAGTAG

hashes:
3803575096664776721
9135176141011874131
6216331087513290446
11066307787392793569
434135890794165763
2161822402243493222

*/

#define SEQ "GAGTGTCAAACATTCAGACAACAGCAGGGGTGCTCTGGAATCCTATGTGAGGAACAAACATTCAGGCCACAGTAG"
#define SEQLEN 75
#define KSIZE 70
#define NUMHASH 3

#define ERR_init "could not init an ntHash iterator"
#define ERR_destroy "could not destroy the iterator"
#define ERR_wrongHash "incorrect hash value produced by ntHash iterator"
#define ERR_multiHash "multihash did not produce enough hash values"

int tests_run = 0;

uint64_t expectedHashes[6] = {
    3803575096664776721,
    9135176141011874131,
    6216331087513290446,
    11066307787392793569,
    434135890794165763,
    2161822402243493222,
};

/*
  test the nthash iterator initialisation and destruction
*/
static char *
test_initNT()
{
    // init
    nthash_iterator_t *nt = ntInit(SEQ, SEQLEN, KSIZE, 1);
    if (nt == NULL)
    {
        return ERR_init;
    }

    // clean up the test
    if (ntDestroy(nt) != 0)
    {
        return ERR_destroy;
    }
    return 0;
}

/*
  test the nthash iteration
*/
static char *test_iterateNT()
{
    // init
    nthash_iterator_t *nt = ntInit(SEQ, SEQLEN, KSIZE, 1);
    if (nt == NULL)
    {
        return ERR_init;
    }

    int testCounter = 0;

    while (!nt->end)
    {
        // check hash k-mer against expected
        if (nt->hashVector[0] != expectedHashes[testCounter])
        {
            return ERR_wrongHash;
        }

        ntIterate(nt);
        testCounter++;
    }

    // clean up the test
    if (ntDestroy(nt) != 0)
    {
        return ERR_destroy;
    }
    return 0;
}

/*
  test the nthash iteration with multiple hash values
*/
static char *test_multihashNT()
{
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

    return 0;
}

/*
  helper function to run all the tests
*/
static char *all_tests()
{
    mu_run_test(test_initNT);
    mu_run_test(test_iterateNT);
    mu_run_test(test_multihashNT);
    return 0;
}

/*
  entrypoint
*/
int main(int argc, char **argv)
{
    fprintf(stderr, "\t\tnthash_test...");
    char *result = all_tests();
    if (result != 0)
    {
        fprintf(stderr, "failed\n");
        fprintf(stderr, "\ntest function %d failed:\n", tests_run);
        fprintf(stderr, "%s\n", result);
    }
    else
    {
        fprintf(stderr, "passed\n");
    }
    return result != 0;
}

#endif