#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "3rd-party/slog.h"
#include "3rd-party/kseq.h"

#include "sketch.h"
#include "sequence.h"
#include "watcher.h"

//TODO: these are to be calculated and stored by antman
#define REF_LENGTH 18246

KSEQ_INIT(gzFile, gzread)
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

// processRef
void processRef(char *filepath, bloomfilter_t *bf, int kSize, int sketchSize)
{
    gzFile fp;
    kseq_t *seq;
    int l;
    fp = gzopen(filepath, "r");
    seq = kseq_init(fp);
    while ((l = kseq_read(seq)) >= 0)
    {

        // add the reference k-mers to the bloom filter
        sketchSequence(seq->seq.s, l, kSize, sketchSize, bf, NULL);

        slog(0, SLOG_LIVE, "\t- processed sequence");
        slog(0, SLOG_LIVE, "\t\t* sequence: %s", seq->name.s);
        slog(0, SLOG_LIVE, "\t\t* length: %d", l);
        slog(0, SLOG_LIVE, "\t\t* %d-mers: %d", kSize, (l - kSize + 1));
    }
    kseq_destroy(seq);

    // check for EOF
    if (l != -1)
    {
        slog(0, SLOG_ERROR, "EOF error for reference file: %d", l);
    }
    gzclose(fp);
    return;
}

// processFastq
void processFastq(void *args)
{
    // get the job
    watcherJob_t *job;
    job = (watcherJob_t *)args;

    // open the sequence file
    gzFile fp;
    kseq_t *seq;
    int l;
    fp = gzopen(job->filepath, "r");
    seq = kseq_init(fp);

    // process each sequence in the fastq file
    while ((l = kseq_read(seq)) >= 0)
    {

        slog(0, SLOG_INFO, "got sequence: %s\n", seq->name.s);
        //if (seq->comment.l) printf("comment: %s\n", seq->comment.s);
        //slog(0, SLOG_INFO, "seq: %s\n;len: %d\n", seq->seq.s, l);
        //if (seq->qual.l) printf("qual: %s\n", seq->qual.s);
    }
    kseq_destroy(seq);

    // check for EOF
    if (l != -1)
    {
        slog(0, SLOG_ERROR, "EOF error for FASTQ file: %d\n", l);
    }
    gzclose(fp);

    if (wjobDestroy(job))
        slog(0, SLOG_ERROR, "failed to destroy job for FASTQ file: %d\n", l);
    return;
}
