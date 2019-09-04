// based on sketch.c from Minimap2
// https://github.com/lh3/minimap2

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "hashmap.h"
#include "heap.h"
#include "slog.h"

unsigned char seq_nt4_table[256] = {
	0, 1, 2, 3,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  3, 3, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  3, 3, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4
};

static inline uint64_t hash64(uint64_t key, uint64_t mask)
{
	key = (~key + (key << 21)) & mask; // key = (key << 21) - key - 1;
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8)) & mask; // key * 265
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4)) & mask; // key * 21
	key = key ^ key >> 28;
	key = (key + (key << 31)) & mask;
	return key;
}

// sketchRead takes the read sequence, the sequence length, the k-mer size and the sketch size
// it generates a MinHash KMV sketch of the read
void sketchRead(const char *str, int len, int k, int sketchSize) {

	// TODO: sketchSize must be < HASHMAP_SIZE,
	// either need checks to make sure this is correct
	// or reimplement to have dynamic allocation for HASHMAP
	assert(sketchSize < HASHMAP_SIZE);

    // check k-mer size and seq length
	assert(len > 0 && (k > 0 && k <= 31) && k <= len);

    // declare the variables
	uint64_t shift1 = 2 * (k - 1), mask = (1ULL<<2*k) - 1, kmer[2] = {0,0}, hashedKmer = 0;
	int i , l, kmer_span = 0;

    // set up the heap for the sketch
    Node* kmvSketch;
	int currentHeapSize = 0;

    // iterate over the sequence
	for (i = l = 0; i < len; i++) {

        // lookup base
		int c = seq_nt4_table[(uint8_t)str[i]];

        // only accept a/c/t/g
		if (c < 4) {
			int z;
            kmer_span = l + 1 < k? l + 1 : k;

            // get the forward and reverse k-mers
			kmer[0] = (kmer[0] << 2 | c) & mask;
			kmer[1] = (kmer[1] >> 2) | (3ULL^c) << shift1;

            // skip symmetrical k-mers
			if (kmer[0] == kmer[1]) continue;
			z = kmer[0] < kmer[1]? 0 : 1; // strand
			l++;

            // hash the canonical k-mer
			if (l >= k && kmer_span < 256) {
				hashedKmer = hash64(kmer[z], mask) << 8 | kmer_span;
			}
		} else l = 0, kmer_span = 0;
        if (i < k) continue;

		// now we have a hashed k-mer, first check if the sketch isn't at capacity yet
		if (currentHeapSize < sketchSize) {

			// check if the hashed k-mer is already in the sketch
			if (hmSearch(hashedKmer)) continue;

			// add the hashed k-mer to the sketch and the tracker
			if (currentHeapSize == 0) {
				  kmvSketch = initHeap(hashedKmer); // special case for first minimum in sketch, which is needed to init the heap
			} else {
				push(&kmvSketch, hashedKmer);
			}
			assert(hmInsert(hashedKmer) == true);
			currentHeapSize++;
			continue;
		}

		// continue if the current max is smaller than the new hashed k-mer
		if (peek(&kmvSketch) <= hashedKmer) continue;

		// continue if the hashed k-mer is already in the current sketch
		if (hmSearch(hashedKmer)) continue;

		// otherwise, the final option is to pop the current max from the sketch and add in the new hashed k-mer
		hmDelete(peek(&kmvSketch));
		pop(&kmvSketch);
		push(&kmvSketch, hashedKmer);
		hmInsert(hashedKmer);
	}

	// the read has now been sketched
	uint64_t* sketchValues = getSketch(&kmvSketch, sketchSize);


	// tmp print loop
	//printf("sketched read:\n");
	//int tmp;
	//for (tmp = 0; tmp < sketchSize; tmp++) {
		//printf("%llu ", sketchValues[tmp]);
	//}
	//printf("\n");
	free(sketchValues);

	// finally, free the guff
	destroy(&kmvSketch);
	hmDestroy();
}