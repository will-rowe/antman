// heap is a heap implementation using a linked list
// the heap is represents the KMV MinHash sketch, containing a subset of hashed k-mers
#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>

// Node is the holder for a minimum (hashed k-mer) in the heap (KMV MinHash sketch)
typedef struct node { 
    struct node* next; 
    uint64_t minimum; // the hashed k-mer
} Node;

/*
    function prototypes
*/
Node* initHeap(uint64_t minimum);
uint64_t peek(Node** head);
void pop(Node** head);
void push(Node** head, uint64_t minimum);
uint64_t* getSketch(Node** head, int numValues);
bool isEmpty(Node** head);
void destroy(Node** head);

#endif