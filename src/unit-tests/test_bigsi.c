#ifndef TEST_BIGSI
#define TEST_BIGSI

#include <stdlib.h>
#include <stdio.h>

#include "minunit.h"
#include "../bigsi.h"
#include "../map.h"

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
#define ERR_query_init "can't assign memory for query result"
#define ERR_query "query failed"
#define ERR_query_fp "bigsi false positive"
#define ERR_query_fn "bigsi false negative"

int tests_run = 0;

/*
  test the BIGSI initialisation and destruction
*/
static char *test_initBIGSI()
{
  // init a BIGSI
  bigsi_t *tmp = initBIGSI(BIT_NUM, HASH_NUM);
  if (tmp == 0 || tmp->numBits != BIT_NUM)
  {
    return ERR_init;
  }

  // clean up the test
  destroyBIGSI(tmp);
  if (tmp->numBits != 0)
  {
    return ERR_destroy;
  }
  return 0;
}

/*
  test the BIGSI insert and index
*/
static char *test_insertBIGSI()
{

  // 3 dummy k-mers
  char kmerA[3] = "act";
  char kmerB[3] = "ggg";
  char kmerC[3] = "cgt";
  char kmerD[3] = "ccc";

  // create 3 sequence bloom filters
  struct bloom bloomA;
  struct bloom bloomB;
  struct bloom bloomC;
  if ((bloom_init(&bloomA, 1000, 0.01) != 0) || (bloom_init(&bloomB, 1000, 0.01) != 0) || (bloom_init(&bloomC, 1000, 0.01) != 0))
  {
    return ERR_bloomfilter;
  }
  bloom_add(&bloomA, kmerA, 3);
  bloom_add(&bloomB, kmerB, 3);
  bloom_add(&bloomC, kmerC, 3);
  if ((!bloom_check(&bloomA, kmerA, 3)) || (!bloom_check(&bloomB, kmerB, 3)) || (!bloom_check(&bloomC, kmerC, 3)))
  {
    return ERR_bloomfilter_2;
  }

  // init a BIGSI
  bigsi_t *bigsi = initBIGSI(bloomA.bits, bloomA.hashes);

  // check that an index operation can't be performed before bit vectors have been added
  if (indexBIGSI(bigsi) != 1)
  {
    return ERR_empty_index;
  }

  // create map for bit vectors and add one
  map_uchar_t bvMap;
  map_init(&bvMap);
  map_set(&bvMap, "kmerA", bloomA.bf);

  // insert a single bit vector
  if (insertBIGSI(bigsi, bvMap, 1) != 0)
  {
    return ERR_insert;
  }

  // check you can't insert the same seqID
  if (insertBIGSI(bigsi, bvMap, 1) != 1)
  {
    return ERR_duplicate;
  }

  // add a couple more seqIDs, then check colours
  map_remove(&bvMap, "kmerA");
  map_set(&bvMap, "kmerB", bloomB.bf);
  map_set(&bvMap, "kmerC", bloomC.bf);
  if (insertBIGSI(bigsi, bvMap, 2) != 0)
  {
    return ERR_insert;
  }
  if (bigsi->colourIterator != 3)
  {
    return ERR_iterator;
  }
  bloom_free(&bloomA);
  bloom_free(&bloomB);
  bloom_free(&bloomC);

  // run the indexing function
  if (indexBIGSI(bigsi) != 0)
  {
    return ERR_empty_index;
  }

  // get the result ready
  unsigned char *result = NULL;
  if ((result = calloc(bigsi->numColourBytes, sizeof(result))) == NULL)
  {
    return ERR_query_init;
  }

  // run the query function on a k-mer which isn't in BIGSI
  if (queryBIGSI(bigsi, kmerD, 3, result))
  {
    return ERR_query;
  }
  if (*result != 0)
  {
    return ERR_query_fp;
  }

  // reassign mem for result
  if ((result = calloc(bigsi->numColourBytes, sizeof(result))) == NULL)
  {
    return ERR_query_init;
  }

  // run the query function on a k-mer that is in BIGSI
  if (queryBIGSI(bigsi, kmerB, 3, result))
  {
    return ERR_query;
  }
  if (*result == 0)
  {
    return ERR_query_fn;
  }

  // check the colour matches the seqID
  for (int i = 0; i < bigsi->numColourBytes; i++)
  {
    int mask = 0x01; /* 00000001 */
    for (int j = 0; j < 8; j++)
    {
      if (result[i] & mask)
      {
        if (strcmp(bigsi->colourArray[i + j], "kmerB"))
        {
          fprintf(stderr, "false positive for colour: %d\n", i + j);
          fprintf(stderr, "translates to %s, expected kmerB\n", bigsi->colourArray[i + j]);
          return ERR_query_fp;
        }
      }
      mask <<= 1; /* move the bit up */
    }
  }

  // clean up the test
  destroyBIGSI(bigsi);
  free(result);
  map_deinit(&bvMap);
  return 0;
}

/*
  helper function to run all the tests
*/
static char *all_tests()
{
  mu_run_test(test_initBIGSI);
  mu_run_test(test_insertBIGSI);
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