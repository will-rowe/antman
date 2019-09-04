#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include "slog.h"
#include "kseq.h"
#include "sketch.h"
#include "sequence.h"

//TODO: these are to be set by user
#define K_SIZE 7
#define SKETCH_SIZE 42

KSEQ_INIT(gzFile, gzread)

// sketchFastq
void sketchFastq(void* arg) {
    const char* filepath = (char*)arg;

    gzFile fp;
    kseq_t *seq;
    int l;

    fp = gzopen(filepath, "r");
    seq = kseq_init(fp);
    while ((l = kseq_read(seq)) >= 0) {
        //slog(0, SLOG_INFO, "name: %s\n", seq->name.s);
        //if (seq->comment.l) printf("comment: %s\n", seq->comment.s);
        //slog(0, SLOG_INFO, "seq: %s\n;len: %d\n", seq->seq.s, l);
        //if (seq->qual.l) printf("qual: %s\n", seq->qual.s);

        // sketch the read
        sketchRead(seq->seq.s, l, K_SIZE, SKETCH_SIZE);
    }
    kseq_destroy(seq);

    // check for EOF
    if (l != -1) {
        slog(0, SLOG_ERROR, "EOF error for FASTQ file: %d\n", l);
    }
    
    gzclose(fp);
    return;
}











/*
// fp rate for bf
double false_positive = 0.001;


double containment_jaccard_estimate(int sequence1_size, string sequence2, vector <string> sketch2, bloom_filter filter) {

    int intersections = 0;
    int sketch_size = sketch2.size();
    for (int i = 0; i < sketch_size; i++) {
        if (filter.contains(sketch2[i])) {
            intersections++;
        }
    }
    intersections -= false_positive * sketch_size;

    double containment_estimate = ((double)intersections / sketch_size);

    int size_sequence1_set = sequence1_size - kmer_size + 1;
    int size_sequence2_set = sequence2.size() - kmer_size + 1;

    return ((double)(containment_estimate * size_sequence2_set)) / (size_sequence1_set + size_sequence2_set - size_sequence2_set * containment_estimate);
}

*/