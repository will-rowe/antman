
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "3rd-party/slog.h"
#include "3rd-party/kseq.h"

#include "errors.h"
#include "nthash.h"
#include "sequence.h"
#include "watcher.h"

KSEQ_INIT(gzFile, gzread)

// processFastq
void processFastq(void *args)
{
    // get the job from the watcher
    watcherJob_t *job;
    config_t *config;
    job = (watcherJob_t *)args;
    config = (config_t *)job->wargs->config;

    // open the sequence file
    gzFile fp;
    kseq_t *seq;
    int seqLen;
    fp = gzopen(job->filepath, "r");
    seq = kseq_init(fp);

    // process each sequence in the fastq file
    while ((seqLen = kseq_read(seq)) >= 0)
    {

        slog(0, SLOG_INFO, "got sequence: %s", seq->name.s);
        //if (seq->comment.l) printf("comment: %s\n", seq->comment.s);
        //slog(0, SLOG_INFO, "seq: %s\n;len: %d\n", seq->seq.s, l);
        //if (seq->qual.l) printf("qual: %s\n", seq->qual.s);

        // set up the hasher
        nthash_iterator_t *nt = ntInit(seq->seq.s, seqLen, config->kSize, config->numHashes);
        if (nt == NULL)
        {
            slog(0, SLOG_ERROR, "could not initiate ntHash hasher");
            kseq_destroy(seq);
            gzclose(fp);
            return;
        }

        // iterate over the k-mers and collect the hash(es)
        while (!nt->end)
        {
            // setup a bitvector to collect results from the bigsi
            bitvector_t *result = bvInit(config->bigsi->colourIterator);
            if (result == NULL)
            {
                slog(0, SLOG_ERROR, "could not init a bit vector for querying the BIGSI");
                ntDestroy(nt);
                kseq_destroy(seq);
                gzclose(fp);
                return;
            }

            // run the query for this k-mer
            int err = 0;
            if ((err = bigsQuery(config->bigsi, nt->hashVector, nt->numHashes, result)) != 0)
            {
                slog(0, SLOG_ERROR, "could not query the BIGSI: %s", printError(err));
                ntDestroy(nt);
                kseq_destroy(seq);
                gzclose(fp);
                return;
            }

            // process the hits for this k-mer
            slog(0, SLOG_LIVE, "query returned %d hits in the BIGSI", result->count);

            ntIterate(nt);
        }

        // clean up the hasher for this sequence
        ntDestroy(nt);
    }

    // finished processing sequences from this file
    kseq_destroy(seq);
    gzclose(fp);

    // check for EOF
    if (seqLen != -1)
    {
        slog(0, SLOG_ERROR, "EOF error for FASTQ file");
        return;
    }

    if (wjobDestroy(job))
        slog(0, SLOG_ERROR, "failed to destroy job for FASTQ file (%s)", job->filepath);
    return;
}
