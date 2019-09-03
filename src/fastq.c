#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include "slog.h"
#include "kseq.h"
#include "sketch.h"

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
        slog(0, SLOG_INFO, "name: %s\n", seq->name.s);
        //if (seq->comment.l) printf("comment: %s\n", seq->comment.s);
        //slog(0, SLOG_INFO, "seq: %s\n;len: %d\n", seq->seq.s, l);
        //if (seq->qual.l) printf("qual: %s\n", seq->qual.s);
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