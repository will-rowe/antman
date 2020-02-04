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
    watcherArgs_t *wargs;
    wargs = (watcherArgs_t *)args;
    gzFile fp;
    kseq_t *seq;
    int l;
    fp = gzopen(wargs->filepath, "r");
    seq = kseq_init(fp);

    // process each sequence in the fastq file
    while ((l = kseq_read(seq)) >= 0)
    {

        //slog(0, SLOG_INFO, "name: %s\n", seq->name.s);
        //if (seq->comment.l) printf("comment: %s\n", seq->comment.s);
        //slog(0, SLOG_INFO, "seq: %s\n;len: %d\n", seq->seq.s, l);
        //if (seq->qual.l) printf("qual: %s\n", seq->qual.s);

        // sketch the read
        uint64_t *sketch = calloc(wargs->sketch_size, sizeof(uint64_t));
        if (!sketch)
        {
            slog(0, SLOG_ERROR, "could not allocate a sketch");
            exit(1);
        }
        sketchSequence(seq->seq.s, l, wargs->kSize, wargs->sketch_size, NULL, sketch);
        slog(0, SLOG_LIVE, "\t- [sketcher]:\tsketched a %dbp sequence", l);

        // estimate read containment within the reference
        // lock the thread whilst using the bloom filter
        int intersections = 0, i;
        pthread_mutex_lock(&mutex1);
        for (i = 0; i < wargs->sketch_size; i++)
        {
            uint8_t result = 0;
            if (bfQuery(wargs->bloomFilter, &*(sketch + i), wargs->kSize, &result) != 0)
            {
                fprintf(stderr, "could not query bloom filter\n");
                return;
            }
            if (result == 1)
            {
                intersections++;
            }
        }
        pthread_mutex_unlock(&mutex1);

        intersections -= (int)floor(wargs->fp_rate * wargs->sketch_size);
        double containmentEstimate = ((double)intersections / wargs->sketch_size);

        int refTotalKmers = REF_LENGTH - wargs->kSize + 1;
        int queryTotalKmers = l - wargs->kSize + 1;

        //slog(0, SLOG_INFO, "%d\t%d\t%d\t%f", intersections, refTotalKmers, queryTotalKmers, containmentEstimate);

        double jaccardEst = ((double)(queryTotalKmers * containmentEstimate)) / ((queryTotalKmers + refTotalKmers) - (queryTotalKmers * containmentEstimate));

        slog(0, SLOG_LIVE, "\t- [sketcher]:\tjaccardEst by containment = %f", jaccardEst);

        free(sketch);
    }
    kseq_destroy(seq);

    // check for EOF
    if (l != -1)
    {
        slog(0, SLOG_ERROR, "EOF error for FASTQ file: %d\n", l);
    }

    gzclose(fp);
    free(wargs);
    return;
}
