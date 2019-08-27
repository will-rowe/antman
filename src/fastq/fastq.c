#include <stdio.h>
#include <stdlib.h>

#include "../log/slog.h"
#include "kseq.h"
KSEQ_INIT(FILE*, read)



// printFastq
void printFastq(char *filepath) {

    // tmp: notify that a thread has received a file
    slog(0, SLOG_INFO, "\t- a worker received the file ---> %s", filepath);

    // open the fastq file
	FILE* fp = fopen(filepath, "r");
	if (fp == 0) {
		slog(0, SLOG_ERROR, "\t- failed to open file ---> %s", filepath);
		exit(1);
	}

    // set up the k-mer info
    int k = 31;
    if (k > 128 || k < 2){
        fprintf(stderr, "ERROR: k must be between 2 and 128\n");
        exit(1);
    }
    char *kmer = malloc(k*sizeof(char));



    // init the reader
	kseq_t *seq = kseq_init(fileno(fp));

    // iterate over each sequence
    unsigned i, j;
    while (kseq_read(seq) >= 0) {

        //  convert to uppercase
        for (i = 0; i < seq->seq.l; ++i) seq->seq.s[i] = toupper(seq->seq.s[i]);

        // count k-mers
        for (i = 0; i < seq->seq.l - k + 1; ++i){
            for (j = 0; j < k; ++j){
                kmer[j] = seq->seq.s[i+j];
            }
            slog(0, SLOG_INFO, "\t- kmer ---> %s", kmer);
        }
    }
    kseq_destroy(seq);
	fclose(fp);
}
