#ifndef TEST_SKETCH
#define TEST_SKETCH

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "minunit.h"
#include "../sketch.c"
#include "../hashmap.c"
#include "../heap.c"
#include "../bloom.c"
#include "../murmurhash2.c"

#define ERR_sketchRead1 "could not sketch read"
#define ERR_initHashMap1 "hashmap was overfilled"
#define ERR_initHashMap2 "value not added to hashmap"
#define ERR_initHashMap3 "value not found in map prior to delete from hashmap"
#define ERR_initHashMap4 "hashmap did not empty"
#define ERR_bloomfilter "could not init bloom filter"
#define ERR_bloomfilter1 "bf should read false for any check when no elements have been added yet"
#define ERR_bloomfilter2 "bf should not produce false negatives"
#define ERR_bloomfilter3 "bf has returned a false positive (which does happen...)"
#define ERR_sketch1 "bf did not return k-mer known to be in the sequence (fn)"
#define ERR_sketch2 "bf returned k-mer known to not be in the sequence (fp)"
#define ERR_alloc "could not allocate"

int tests_run = 0;

// jenkins hash function for the bloom filter
unsigned int jenkins(const void *_str)
{
  const char *key = _str;
  unsigned int hash = 0;
  while (*key)
  {
    hash += *key;
    hash += (hash << 10);
    hash ^= (hash >> 6);
    key++;
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}

/*
  test the hashmap
*/
static char *test_hashmap()
{

  // create a hashmap and fill it to capacity
  uint64_t i;
  for (i = 0; i < HASHMAP_SIZE; i++)
  {
    if (!hmInsert(i))
    {
      return ERR_initHashMap2;
    }
  }

  // make sure hashmap can't be overfilled
  if (hmInsert(HASHMAP_SIZE))
  {
    return ERR_initHashMap1;
  }

  // check that the values are in the map
  for (i = 0; i < HASHMAP_SIZE; i++)
  {
    if (!hmSearch(i))
    {
      return ERR_initHashMap2;
    }
  }

  // delete values from the hashmap
  for (i = 0; i < HASHMAP_SIZE; i++)
  {
    hmDelete(i);
  }

  // check the hashmap is empty now all values were deleted
  for (i = 0; i < HASHMAP_SIZE; i++)
  {
    if (hmSearch(i))
    {
      return ERR_initHashMap4;
    }
  }
  hmDestroy();
  return 0;
}

/*
  test the bloom filter
*/
static char *test_bloomfilter()
{
  char *testString1 = "hello, world!";
  char *testString2 = "world, hello!";
  int testStringLen = 13;

  struct bloom bloom;
  if (bloom_init(&bloom, 1000000, 0.01) != 0)
  {
    return ERR_bloomfilter;
  }
  if (bloom_check(&bloom, testString1, testStringLen))
  {
    return ERR_bloomfilter1;
  }
  bloom_add(&bloom, testString1, testStringLen);
  if (!bloom_check(&bloom, testString1, testStringLen))
  {
    return ERR_bloomfilter2;
  }
  if (bloom_check(&bloom, testString2, testStringLen))
  {
    return ERR_bloomfilter3;
  }
  bloom_free(&bloom);
  return 0;
}

/*
  test the sequence sketching
*/
static char *test_sketchSeq()
{
  struct bloom bloom;
  if (bloom_init(&bloom, 1000000, 0.01) != 0)
  {
    return ERR_bloomfilter;
  }

  // sketch a sequence
  char *seq = "actgactgactg";
  int seqLen = 12;
  int kSize = 3;
  int sketchSize = 4;
  uint64_t hashedKmer = 14595;
  uint64_t dummyHashedKmer = 14596;
  uint64_t *sketch = calloc(3, sizeof(uint64_t));
  if (!sketch)
  {
    return ERR_alloc;
  }
  sketchSequence(seq, seqLen, kSize, sketchSize, &bloom, sketch);

  // confirm the bloom filter worked
  if (!bloom_check(&bloom, &hashedKmer, kSize))
  {
    return ERR_sketch1;
  }
  if (bloom_check(&bloom, &dummyHashedKmer, kSize))
  {
    return ERR_sketch2;
  }

  // TODO: validate the sketch

  free(sketch);
  bloom_free(&bloom);
  return 0;
}

/*
  helper function to run all the tests
*/
static char *all_tests()
{
  mu_run_test(test_hashmap);
  mu_run_test(test_bloomfilter);
  mu_run_test(test_sketchSeq);
  return 0;
}

/*
  entrypoint
*/
int main(int argc, char **argv)
{
  fprintf(stderr, "\t\tsketch_test...");
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