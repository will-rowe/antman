/*****************************************************************************
 * Package heap is a heap implementation using a linked list.
 * 
 * We're using a heap to represent the KMV MinHash sketch, which contains a
 * subset of hashed k-mers.
 */
#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************
 * node_t is a heap element and represents a value in the sketch (the hashed k-mer)
 */
typedef struct node node_t;

/*****************************************************************************
 * function prototypes
 */
node_t *initHeap(uint64_t minimum);
uint64_t peek(node_t **head);
void pop(node_t **head);
void push(node_t **head, uint64_t minimum);
void getSketch(node_t **head, int numValues, uint64_t *sketch);
bool isEmpty(node_t **head);
void destroy(node_t **head);

#endif