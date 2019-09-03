#ifndef TEST_SKETCH
#define TEST_SKETCH

#include <stdint.h>
#include <stdio.h>

#include "minunit.h"
#include "../src/sketch.c"
#include "../src/hashmap.c"
#include "../src/hashmap.h"
#include "../src/heap.c"
#include "../src/heap.h"

#define ERR_sketchRead1 "could not sketch read"
#define ERR_initHashMap1 "hashmap was overfilled"
#define ERR_initHashMap2 "value not added to hashmap"
#define ERR_initHashMap3 "value not found in map prior to delete from hashmap"
#define ERR_initHashMap4 "hashmap did not empty"


int tests_run = 0;

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

  return 0;
}

/*
  helper function to run all the tests
*/
static char* all_tests() {
  mu_run_test(test_hashmap);
  mu_run_test(test_sketchRead);
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