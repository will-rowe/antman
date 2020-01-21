#ifndef TEST_BLOOMFILTER
#define TEST_BLOOMFILTER

#include <stdlib.h>
#include <stdio.h>

#include "minunit.h"
#include "../bloomfilter.h"

#define CAPACITY 2000
#define COLLISION_PROB 0.01
#define ERR_init "could not init a bloom filter"
#define ERR_destroy "could not destroy the bloom filter"
#define ERR_add "could not add entry to bloom filter"
#define ERR_bitset "incorrect number of bits set"
#define ERR_query "could not query the bloom filter"
#define ERR_fn "bloom filter returned false negative"
#define ERR_fp "bloom filter returned false positive"

int tests_run = 0;

/*
  test the bloom filter initialisation and destruction
*/
static char *test_initBF()
{
    // init a bloom filter
    bloomfilter_t *bf = bfInit(CAPACITY, COLLISION_PROB);
    if (bf == NULL)
    {
        return ERR_init;
    }

    // clean up the test
    if (bfDestroy(bf) != 0)
    {
        return ERR_destroy;
    }
    return 0;
}

/*
  test the bloom filter add and query funcs
*/
static char *test_addquery()
{
    // init a bloom filter
    bloomfilter_t *bf = bfInit(CAPACITY, COLLISION_PROB);
    if (bf == NULL)
    {
        return ERR_init;
    }

    // add
    char kmerA[] = "act";
    if (bfAdd(bf, kmerA, 3) != 0)
    {
        return ERR_add;
    }
    if (bvCount(bf->bitvector) != bf->numHashes)
    {
        return ERR_bitset;
    }

    // query 1
    uint8_t result = 0;
    if (bfQuery(bf, kmerA, 3, &result) != 0)
    {
        return ERR_add;
    }

    // check for false negative
    if (result != 1)
    {
        return ERR_fn;
    }

    // query 2
    result = 0;
    char kmerB[] = "ggg";
    if (bfQuery(bf, kmerB, 3, &result) != 0)
    {
        return ERR_add;
    }

    // check for false negative
    if (result == 1)
    {
        return ERR_fp;
    }

    // clean up the test
    if (bfDestroy(bf) != 0)
    {
        return ERR_destroy;
    }
    return 0;
}

/*
  helper function to run all the tests
*/
static char *all_tests()
{
    mu_run_test(test_initBF);
    mu_run_test(test_addquery);
    return 0;
}

/*
  entrypoint
*/
int main(int argc, char **argv)
{
    fprintf(stderr, "\t\tbloomfilter_test...");
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