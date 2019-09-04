#ifndef TEST_SKETCH
#define TEST_SKETCH

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "minunit.h"
#include "../src/sketch.c"
#include "../src/hashmap.c"
#include "../src/heap.c"
#include "../src/bloom.c"
#include "../src/murmurhash2.c"

#define ERR_sketchRead1 "could not sketch read"
#define ERR_initHashMap1 "hashmap was overfilled"
#define ERR_initHashMap2 "value not added to hashmap"
#define ERR_initHashMap3 "value not found in map prior to delete from hashmap"
#define ERR_initHashMap4 "hashmap did not empty"
#define ERR_bloomfilter1 "bf should read false for any check when no elements have been added yet"
#define ERR_bloomfilter2 "bf should not produce false negatives"
#define ERR_bloomfilter3 "bf has returned a false positive (which does happen...)"

int tests_run = 0;

// jenkins hash function for the bloom filter
unsigned int jenkins(const void *_str) {
	const char *key = _str;
	unsigned int hash, i;
	while (*key) {
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
static char* test_hashmap() {

  // create a hashmap and fill it to capacity
  uint64_t i;
  for (i = 0; i < HASHMAP_SIZE; i++) {
    if (!hmInsert(i)) {
      return ERR_initHashMap2;
    }
  }

  // make sure hashmap can't be overfilled
  if (hmInsert(HASHMAP_SIZE)) {
    return ERR_initHashMap1;
  }

  // check that the values are in the map
  for (i = 0; i < HASHMAP_SIZE; i++) {
    if (!hmSearch(i)) {
      return ERR_initHashMap2;
    }
  }

  // delete values from the hashmap
  for (i = 0; i < HASHMAP_SIZE; i++) {
    hmDelete(i);
  }

  // check the hashmap is empty now all values were deleted
  for (i = 0; i < HASHMAP_SIZE; i++) {
    if (hmSearch(i)) {
      return ERR_initHashMap4;
    }
  }
  hmDestroy();
  return 0;
}

/*
  test the read sketching
*/
static char* test_sketchRead() {
  char* read = "actgactgactg";
  sketchRead(read, 12, 3, 4);

  // TODO: validate the sketch

  return 0;
}

/*
  test the bloom filter
*/
static char* test_bloomfilter() {
  char* testString1 = "hello, world!";
  char* testString2 = "world, hello!";
  int testStringLen = 13;

  struct bloom bloom;
  bloom_init(&bloom, 1000000, 0.01);
  if (bloom_check(&bloom, testString1, testStringLen)) {
    return ERR_bloomfilter1;
  }
  bloom_add(&bloom, testString1, testStringLen);
  if (!bloom_check(&bloom, testString1, testStringLen)) {
    return ERR_bloomfilter2;
  }
  if (bloom_check(&bloom, testString2, testStringLen)) {
    return ERR_bloomfilter3;
  }

  bloom_free(&bloom);
  return 0;
}

/*
  helper function to run all the tests
*/
static char* all_tests() {
  mu_run_test(test_hashmap);
  mu_run_test(test_sketchRead);
  mu_run_test(test_bloomfilter);
  return 0;
}

/*
  entrypoint
*/
int main(int argc, char **argv) {
  fprintf(stderr, "\t\tsketch_test...");
  char *result = all_tests();
  if (result != 0) {
    fprintf(stderr, "failed\n");
    fprintf(stderr, "\ntest function %d failed:\n", tests_run);
    fprintf(stderr, "%s\n", result);
  } else {
    fprintf(stderr, "passed\n");
  }
  return result != 0;
}

#endif