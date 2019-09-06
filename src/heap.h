// heap is a heap implementation using a linked list
// the heap is represents the KMV MinHash sketch, containing a subset of hashed k-mers
#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>

/*
    node_t contains information for a minimum (hashed k-mer)
    the heap is linked list of nodes and equates to a KMV MinHash sketch
*/
typedef struct node node_t;

/*
    function prototypes
*/
node_t* initHeap(uint64_t minimum);
uint64_t peek(node_t** head);
void pop(node_t** head);
void push(node_t** head, uint64_t minimum);
void getSketch(node_t** head, int numValues, uint64_t* sketch);
bool isEmpty(node_t** head);
void destroy(node_t** head);

#endif