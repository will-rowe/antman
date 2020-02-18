#ifndef TEST_SEQUENCE
#define TEST_SEQUENCE

#include <stdlib.h>
#include <stdio.h>

#include "minunit.h"

#include "../src/3rd-party/map.h"

#include "../src/bigsi.h"
#include "../src/bitvector.h"
#include "../src/bloomfilter.h"
#include "../src/errors.h"
#include "../src/nthash.h"

#define ERR "failed - check other unit tests for details"

#define NUMBITS 200
#define NUMHASHES 10
#define SEQ "GAGTGTCAAACATTCAGACAACAGCAGGGGTGCTCTGGAATCCTATGTGAGGAACAAACATTCAGGCCACAGTAG"
#define SEQLEN 75
#define SEQNAME "testSeq"
#define KSIZE 70

int tests_run = 0;

/*
  test creating a BIGSI from a sequence
*/
static char *test_create()
{
    // init a BIGSI
    bigsi_t *bigsi = bigsInit(NUMBITS, NUMHASHES, ".");
    if (!bigsi)
    {
        fprintf(stderr, "could not init BIGSI");
        return ERR;
    }

    // get bloom filter map ready
    map_bloomfilter_t bfMap;
    map_init(&bfMap);

    // create a bloom filter for the test sequence
    bloomfilter_t *bf;
    if ((bf = bfInitWithSize(NUMBITS, NUMHASHES)) == NULL)
    {
        fprintf(stderr, "could not initiate bloom filter\n");
        return ERR;
    }

    // set up the sequence hasher
    nthash_iterator_t *nt = ntInit(SEQ, SEQLEN, KSIZE, NUMHASHES);
    if (nt == NULL)
    {
        fprintf(stderr, "could not initiate ntHash hasher\n");
        return ERR;
    }

    // iterate over the k-mers and collect the hash(es)
    while (!nt->end)
    {

        // insert the hashes into the bloom filter
        if (bfAddPC(bf, nt->hashVector, NUMHASHES))
        {
            fprintf(stderr, "could not add hashes to bloom filter\n");
            return ERR;
        }
        ntIterate(nt);
    }

    // clean up the hasher
    ntDestroy(nt);

    // copy the bloom filter to map
    map_set(&bfMap, SEQNAME, *bf);

    // copy the map to BIGSI, then free the map and bloom filter
    if (bigsAdd(bigsi, bfMap, 1) != 0)
    {
        fprintf(stderr, "could not add bloom filter to BIGSI");
        return ERR;
    }
    map_deinit(&bfMap);
    bfDestroy(bf);

    // run the indexing function
    if (bigsIndex(bigsi) != 0)
    {
        fprintf(stderr, "could not index BIGSI");
        return ERR;
    }

    // save BIGSI to disk and free the in-memory BIGSI
    if (bigsFlush(bigsi) != 0)
    {
        fprintf(stderr, "could not flush BIGSI");
        return ERR;
    }
    return 0;
}

/*
  test querying a BIGSI for a sequence
*/
static char *test_query()
{
    int errCode = 0;

    // load the BIGSI
    bigsi_t *bigsi;
    if ((bigsi = bigsLoad(".")) == NULL)
    {
        fprintf(stderr, "could not load BIGSI");
        return ERR;
    }

    // get the result ready
    bitvector_t *result = bvInit(bigsi->colourIterator);
    if (result == NULL)
    {
        fprintf(stderr, "could not init a bit vector");
        return ERR;
    }

    // hash the sequence again, this time querying the BIGSI
    nthash_iterator_t *nt = ntInit(SEQ, SEQLEN, KSIZE, NUMHASHES);
    if (nt == NULL)
    {
        fprintf(stderr, "could not initiate ntHash hasher\n");
        return ERR;
    }
    while (!nt->end)
    {
        if ((errCode = bigsQuery(bigsi, nt->hashVector, NUMHASHES, result)) != 0)
        {
            fprintf(stderr, "could not query the BIGSI: %s\n", printError(errCode));
            return ERR;
        }

        // check there was a match for the k-mer
        if (bvCount(result) == 0)
        {
            fprintf(stderr, "BIGSI query failed\n");
            return ERR;
        }

        // wipe the bit vector for the next query
        if (bvClear(result) != 0)
        {
            fprintf(stderr, "could not wipe bit vector\n");
            return ERR;
        }
        ntIterate(nt);
    }

    // clean up the hasher
    ntDestroy(nt);

    // clean up
    bvDestroy(result);
    if (bigsFlush(bigsi) != 0)
    {
        fprintf(stderr, "could not flush BIGSI");
        return ERR;
    }

    return 0;
}

/*
  test the BIGSI load and query
*/
static char *test_cleanup()
{
    remove(BIGSI_METADATA_FILENAME);
    remove(BITVECTORS_DB_FILENAME);
    remove(COLOURS_DB_FILENAME);
    return 0;
}

/*
  helper function to run all the tests
*/
static char *all_tests()
{
    mu_run_test(test_create);
    mu_run_test(test_query);
    mu_run_test(test_cleanup);
    return 0;
}

/*
  entrypoint
*/
int main(int argc, char **argv)
{
    fprintf(stderr, "\t\tbigsi_test...");
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