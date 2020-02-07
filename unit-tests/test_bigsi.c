#ifndef TEST_BIGSI
#define TEST_BIGSI

#include <stdlib.h>
#include <stdio.h>

#include "minunit.h"
#include "../src/bigsi.h"
#include "../src/bitvector.h"
#include "../src/bloomfilter.h"
#include "../src/3rd-party/map.h"

#define BIT_NUM 10
#define HASH_NUM 1
#define ERR_init "could not init a BIGSI"
#define ERR_bloomfilter "could not init a bloom filter"
#define ERR_bloomfilter_2 "could not insert into a bloom filter"
#define ERR_destroy "could not destroy the BIGSI"
#define ERR_insert "could not insert bit vector into BIGSI"
#define ERR_duplicate "allowed duplicate entries into BIGSI"
#define ERR_iterator "iterator and insert mismatch"
#define ERR_empty_index "allowed empty BIGSI to be indexed"
#define ERR_empty_index2 "bigsIndex returned empty index"
#define ERR_bigsi_dump "wrote unindexed bigsi to disk"
#define ERR_bigsi_flushed_unindexed "flushed an unindexed bigsi"
#define ERR_bigsi_flush "could not write bigsi to disk"
#define ERR_bigsi_load "could not load bigsi from disk"
#define ERR_query_init "can't assign memory for query result"
#define ERR_query "query failed"
#define ERR_bitvector_clear "could not clear bit vector"
#define ERR_query_fp "bigsi false positive"
#define ERR_query_fn "bigsi false negative"
#define ERR_bitvector_get "could not run bvGet"
#define ERR_colour_lookup "could not run a colour lookup"

int tests_run = 0;

/*
  test the BIGSI initialisation and destruction
*/
static char *test_bigsInit()
{
  // init a BIGSI
  bigsi_t *tmp = bigsInit(BIT_NUM, HASH_NUM, ".");
  if (tmp == 0 || tmp->numBits != BIT_NUM)
  {
    return ERR_init;
  }

  // clean up the test
  if (bigsDestroy(tmp) != 0)
  {
    return ERR_destroy;
  }
  return 0;
}

/*
  test the BIGSI insert and index

  sequence 1 will contain k-mers A and B (act, ggg)
  sequence 2 will contain k-mer C (cgt)
  neither sequence will contain k-mer D (ccc)

*/
static char *test_bigsIndex()
{

  // 3 dummy k-mers
  char kmerA[] = "act";
  char kmerB[] = "ggg";
  char kmerC[] = "cgt";

  // create 2 sequence bloom filters
  bloomfilter_t *bloomSeq1 = bfInit(2000, 0.01);
  bloomfilter_t *bloomSeq2 = bfInit(2000, 0.01);
  if ((bloomSeq1 == NULL) || (bloomSeq2 == NULL))
  {
    return ERR_bloomfilter;
  }
  bfAdd(bloomSeq1, &kmerA, 3);
  bfAdd(bloomSeq1, &kmerB, 3);
  bfAdd(bloomSeq2, &kmerC, 3);

  // init a BIGSI
  bigsi_t *bigsi = bigsInit(bvCapacity(bloomSeq1->bitvector), bloomSeq1->numHashes, ".");

  // check that an index operation can't be performed before bit vectors have been added
  if (bigsIndex(bigsi) != -1)
  {
    return ERR_empty_index;
  }

  // create map for bit filters and add one
  map_bloomfilter_t bfMap;
  map_init(&bfMap);
  map_set(&bfMap, "sequence 1", *bloomSeq1);

  // insert a single bit vector
  if (bigsAdd(bigsi, bfMap, 1) != 0)
  {
    return ERR_insert;
  }

  // check you can't insert the same seqID
  if (bigsAdd(bigsi, bfMap, 1) != -1)
  {
    return ERR_duplicate;
  }

  // add a couple more seqIDs, then check colours
  map_remove(&bfMap, "sequence 1");
  map_set(&bfMap, "sequence 2", *bloomSeq2);
  if (bigsAdd(bigsi, bfMap, 1) != 0)
  {
    return ERR_insert;
  }
  if (bigsi->colourIterator != 2)
  {
    return ERR_iterator;
  }

  // bloom filters are now copied to BIGSI, delete originals
  bfDestroy(bloomSeq1);
  bfDestroy(bloomSeq2);
  map_deinit(&bfMap);

  // make sure you can't flush before indexing
  if (bigsFlush(bigsi) == 0)
  {
    return ERR_bigsi_flushed_unindexed;
  }

  // run the indexing function
  if (bigsIndex(bigsi) != 0)
  {
    return ERR_empty_index2;
  }

  // save to disk and free
  if (bigsFlush(bigsi) != 0)
  {
    return ERR_bigsi_flush;
  }

  return 0;
}

/*
  test the BIGSI load and query
*/
static char *test_bigsQuery()
{

  // load the previous BIGSI
  bigsi_t *bigsi;
  if ((bigsi = bigsLoad(".")) == NULL)
  {
    return ERR_bigsi_load;
  }

  // set up queries
  //char kmerA[] = "act"; // contained in sequence 1
  char kmerB[] = "ggg"; // contained in sequence 1
  //char kmerC[] = "cgt"; // contained in sequence 2
  char kmerD[] = "ccc"; // not in either sequence
  uint64_t *kmerBhashes = calloc(sizeof(uint64_t), bigsi->numHashes);
  uint64_t *kmerDhashes = calloc(sizeof(uint64_t), bigsi->numHashes);
  for (int i = 0; i < bigsi->numHashes; i++)
  {
    fprintf(stderr, "num bigsi hashes: %u\n\n", bigsi->numHashes);
    kmerBhashes[i] = getHashVal(kmerB, 3, i);
    kmerDhashes[i] = getHashVal(kmerD, 3, i);
  }

  // get the result ready
  bitvector_t *result = bvInit(bigsi->colourIterator);
  if (result == NULL)
  {
    return ERR_query_init;
  }

  // run the query function on a k-mer which isn't in BIGSI
  if (bigsQuery(bigsi, kmerDhashes, bigsi->numHashes, result))
  {
    return ERR_query;
  }
  if (bvCount(result) != 0)
  {
    return ERR_query_fp;
  }

  // wipe the result bit vector
  if (bvClear(result) != 0)
  {
    return ERR_bitvector_clear;
  }

  // run the query function on a k-mer that is in BIGSI
  if (bigsQuery(bigsi, kmerBhashes, bigsi->numHashes, result))
  {
    return ERR_query;
  }
  if (bvCount(result) == 0)
  {
    return ERR_query_fn;
  }

  // iterate over set bits in result
  /*
  TODO: this is not how we will ultimately check results
  */
  int fnCheck = 0;
  for (int colour = 0; colour < result->capacity; colour++)
  {
    uint8_t bitVal = 0;
    if (bvGet(result, colour, &bitVal) != 0)
    {
      return ERR_bitvector_get;
    }
    if (bitVal == 1)
    {
      // get the colour -> seqID
      char *seqID;
      if (bigsLookupColour(bigsi, colour, &seqID) != 0)
      {
        return ERR_colour_lookup;
      }
      if (strcmp(seqID, "sequence 1") != 0)
      {
        return ERR_query_fp;
      }
      else
      {
        fnCheck = 1;
      }
      free(seqID);
    }
  }
  if (fnCheck != 1)
  {
    return ERR_query_fn;
  }

  // clean up the test
  free(kmerBhashes);
  free(kmerDhashes);
  bvDestroy(result);
  if (bigsFlush(bigsi) != 0)
  {
    return ERR_bigsi_flush;
  }
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
  mu_run_test(test_bigsInit);
  mu_run_test(test_bigsIndex);
  mu_run_test(test_bigsQuery);
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