#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"

kmer* hashArray[HASHMAP_SIZE];

// kmer
struct kmer { 
   uint64_t kmerHash;
};

// hmInsert a hashed k-mer into the hashmap
// returns true if inserted, false if not
bool hmInsert(uint64_t kmerHash) {

   // get the initial index position for this hashed k-mer in the hashmap
   int hashIndex = kmerHash % HASHMAP_SIZE;

   // if the array cell at the hashIndex is full, keep moving until an empty one is found or until all cells have been checked
   int counter = 0;
   while (hashArray[hashIndex] != NULL) {
      hashIndex++;
      hashIndex %= HASHMAP_SIZE;

      // if the array is full, return false
      counter++;
      if (counter == HASHMAP_SIZE) {
         return false;
      }
   }

   // store the hashed k-mer
   struct kmer *tmp = (kmer*) malloc(sizeof(struct kmer));
   tmp->kmerHash = kmerHash;
   hashArray[hashIndex] = tmp;
   return true;
}

// hmSearch for a hashed k-mer in the hashmap
// returns true if present, false if absent
bool hmSearch(uint64_t kmerHash) {

   // find the approximate location of the hashed k-mer in the array
   int hashIndex = kmerHash % HASHMAP_SIZE;  

   // if NULL is found before the query, then the query is not in the array or has been deleted
   while(hashArray[hashIndex] != NULL) {
      if(hashArray[hashIndex]->kmerHash == kmerHash)
         return true; 
			
      // go to next cell in the array
      hashIndex++;
		
      // make sure the index wraps around the array
      hashIndex %= HASHMAP_SIZE;
   }
   return false;        
}

// hmDelete will remove a hashed k-mer from the map
void hmDelete(uint64_t kmerHash) {
   int hashIndex = kmerHash % HASHMAP_SIZE;
   while(hashArray[hashIndex] != NULL) {
      if(hashArray[hashIndex]->kmerHash == kmerHash) {
         free(hashArray[hashIndex]);
         hashArray[hashIndex] = NULL;
         break;
      }
      hashIndex++;
      hashIndex %= HASHMAP_SIZE;
   }      
}

// hmDestroy will free the hashmap
void hmDestroy(void) {
   int i;
   for (i = 0; i < HASHMAP_SIZE; i++) {
      free(hashArray[i]);
      hashArray[i] = NULL;      
   }
}