#ifndef TEST_HEAP
#define TEST_HEAP

#include <stdint.h>
#include <stdio.h>

#include "minunit.h"
#include "../src/heap.c"

#define ERR_initHeap1 "could not init a heap"
#define ERR_initHeap2 "could not access heap minimum"
#define ERR_initHeap3 "could not destroy the heap"
#define ERR_minHeap1 "heap not printed"
#define ERR_minHeap2 "heap not min sorted"
#define ERR_alloc "could not allocate"

int tests_run = 0;

/*
  test the heap initialisation and destruction
*/
static char* test_initHeap() {

  // create a heap with one minimum
  node_t* testHeap;
  uint64_t minimum = 1234;
  testHeap = initHeap(minimum);

  // check the heap was created and has a node in it
  if (isEmpty(&testHeap)) {
    return ERR_initHeap1;
  }
  
  // check the node holds the right value
  if (peek(&testHeap) != minimum) {
    return ERR_initHeap2;
  }

  // check the heap can be destroyed
  destroy(&testHeap);
  if (!isEmpty(&testHeap)) {
    return ERR_initHeap3;
  }

  return 0;
}

/*
  test the heap is sorted with smallest minimum at the bottom, largest at the top
*/
static char* test_minHeap() {
  uint64_t valA = 1234, valB = 1, valC = 42;
  node_t* testHeap = initHeap(valA);
  push(&testHeap, valB);
  push(&testHeap, valC);

  // check the heap can be printed and is in order
  uint64_t* heapValues = calloc(3, sizeof(uint64_t));
  if(!heapValues) {
    return ERR_alloc;
  } 
  getSketch(&testHeap, 3, heapValues);
  if (heapValues[0] != valA || heapValues[1] != valC || heapValues[2] != valB) return ERR_minHeap1;
  free(heapValues);

  // check the peek, pop and destroy functions
  if (peek(&testHeap) != valA) {
    return ERR_minHeap2;
  }
  pop(&testHeap);
  if (peek(&testHeap) != valC) {
    return ERR_minHeap2;
  }
  pop(&testHeap);
  if (peek(&testHeap) != valB) {
    return ERR_minHeap2;
  }
  destroy(&testHeap);
  if (!isEmpty(&testHeap)) {
    return ERR_initHeap3;
  }
  return 0;
}

/*
  helper function to run all the tests
*/
static char* all_tests() {
  mu_run_test(test_initHeap);
  mu_run_test(test_minHeap);
  return 0;
}

/*
  entrypoint
*/
int main(int argc, char **argv) {
  fprintf(stderr, "\t\theap_test...");
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