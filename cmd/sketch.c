#include <fcntl.h>
#include <stdio.h>
#include <zlib.h>

#include "../src/3rd-party/kseq.h"
#include "../src/3rd-party/map.h"
#include "../src/3rd-party/slog.h"

#include "../src/config.h"
#include "../src/sketch.h"

KSEQ_INIT(gzFile, gzread)

/*****************************************************************************
 * sketch is the subcommand function.
 * 
 * TODO: set up worker pool
 * TODO: add logging
 * TODO: add checks
 */
int sketch(config_t *config, char *filePath)
{
    // get bloom filter map ready
    map_bloomfilter_t bfMap;
    map_init(&bfMap);

    // open file or get stdin ready
    gzFile fp;
    kseq_t *seq;
    if (filePath)
        fp = gzdopen(open(filePath, O_RDONLY), "r");
    else
        fp = gzdopen(fileno(stdin), "r");
    seq = kseq_init(fp);

    // process sequences
    int seqCounter = 0, kmerCounter = 0, seqLen = 0;
    while ((seqLen = kseq_read(seq)) >= 0)
    {

        // create a bloom filter
        bloomfilter_t *bf;
        if ((bf = bfInitWithSize(config->numBits, config->numHashes)) == NULL)
        {
            fprintf(stderr, "could not initiate bloom filter\n");
            return -1;
        }

        // TODO: segfaults without this.....
        // must be masking something - but am just hacking on this at the moment and will likely change the entire function...
        fprintf(stderr, "bit vector bits: %llu\n\n", bf->numBits);

        // process the sequence
        seqCounter++;
        kmerCounter += (seqLen - config->kSize + 1);
        if (sketchSequence(seq->seq.s, seqLen, config->kSize, 0, bf, NULL))
        {
            fprintf(stderr, "could not sketch reference sequence: %s\n", seq->name.s);
            return -1;
        }

        // add bloom filter to map
        map_set(&bfMap, seq->name.s, *bf);
        bfDestroy(bf);
    }
    slog(0, SLOG_INFO, "\t- processed %u sequences", seqCounter);
    slog(0, SLOG_INFO, "\t- added %u k-mers", kmerCounter);
    slog(0, SLOG_LIVE, "creating reference sketch database...");

    // add bloom filters to BIGSI
    if (bigsAdd(config->bigsi, bfMap, seqCounter) != 0)
        return -1;

    // index the BIGSI
    if (bigsIndex(config->bigsi) != 0)
        return -1;

    // save to disk and free
    if (bigsFlush(config->bigsi) != 0)
        return -1;
    config->bigsi = NULL;

    // cleanup
    map_deinit(&bfMap);
    kseq_destroy(seq);
    gzclose(fp);
    return 0;
}
