#include <stdio.h>
#include <stdlib.h>

#include "slog.h"
#include "kseq.h"


#include <zlib.h>


KSEQ_INIT(gzFile, gzread)
void printFastq(char *filepath) {
  gzFile fp;
  kseq_t *seq;
  int l;

  fp = gzopen(filepath, "r");
  seq = kseq_init(fp);
  while ((l = kseq_read(seq)) >= 0) {
    slog(0, SLOG_INFO, "name: %s\n", seq->name.s);
    //if (seq->comment.l) printf("comment: %s\n", seq->comment.s);
    slog(0, SLOG_INFO, "seq: %s\n", seq->seq.s);
    //if (seq->qual.l) printf("qual: %s\n", seq->qual.s);
  }
  slog(0, SLOG_INFO, "return value: %d\n", l);
  kseq_destroy(seq);
  gzclose(fp);
  return;

}


/*

get canonical k-mer and hash
if sketch isn't full, add
if hash < biggest minimum in sketch AND hash not already in sketch, add

update hash table which is tracking contents of the kmv sketch


when read is sketched, check each minimum against a bloom filter of the reference k-mers


*/


/*
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
*/