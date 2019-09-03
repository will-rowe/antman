#include <stdio.h> 
#include <stdlib.h> 
#include "heap.h"

// initHeap creates a new node in the heap 
Node* initHeap(uint64_t minimum) { 
    Node* tmp = (Node*)malloc(sizeof(Node)); 
    tmp->next = NULL; 
    tmp->minimum = minimum; 
    return tmp; 
} 
  
// peek will return the largest minimum currently in the heap
uint64_t peek(Node** head) {
    return (*head)->minimum; 
}
  
// pop will remove the largest minimum currently in the heap and return it
void pop(Node** head) {
    Node* tmp = *head; 
    (*head) = (*head)->next; 
    free(tmp); 
} 
  
// push will add a minimum to the heap 
void push(Node** head, uint64_t minimum) {

    // create a new node for the incoming minimum 
    Node* tmp = initHeap(minimum); 

    // start with the largest minimum already in the heap
    Node* start = (*head); 

    // if the new minimum > the start node, add it at the top of the heap and change the head node 
    if ((*head)->minimum < minimum) {
        tmp->next = *head; 
        (*head) = tmp; 
    }

    // otherwise, move down the heap and find where the new minimum should be inserted
    else { 
        while (start->next != NULL && start->next->minimum > minimum) { 
            start = start->next; 
        }
        tmp->next = start->next; 
        start->next = tmp; 
    }
}

// getSketch will return the heap values, from top to bottom (largest to smallest)
// if heap < numValues, the remainder will be 0s
uint64_t* getSketch(Node** head, int numValues) {
    uint64_t *returnValues = calloc(numValues, sizeof(uint64_t));
    if(!returnValues) {
        return NULL;
    }
    Node* start = (*head);
    int i = 0;
    while(start->next != NULL) {
        returnValues[i] = start->minimum;
        start = start->next;
        i++;
    }
    // grab the final value and then return
    returnValues[i] = start->minimum;
    return returnValues;
}

// isEmpty checks if the heap is empty
bool isEmpty(Node** head) {
    return (*head) == NULL; 
}

// destroy will free the heap
void destroy(Node** head) {
    Node *tmp;

    // set the tmp node to head and stop the traversal if the list is empty
    while ((tmp = *head) != NULL) { 
        (*head) = (*head)->next;
        free(tmp);
    }
}


