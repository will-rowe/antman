#include <fcntl.h>
#include <stdio.h>
#include <zlib.h>

#include "../src/3rd-party/kseq.h"
#include "../src/3rd-party/map.h"
#include "../src/3rd-party/slog.h"

#include "../src/config.h"
#include "../src/nthash.h"

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

        // create a bloom filter for this sequence
        bloomfilter_t *bf;
        if ((bf = bfInitWithSize(config->numBits, config->numHashes)) == NULL)
        {
            fprintf(stderr, "could not initiate bloom filter\n");
            return -1;
        }

        // record the sequence
        seqCounter++;
        kmerCounter += (seqLen - config->kSize + 1);

        // set up the hasher
        nthash_iterator_t *nt = ntInit(seq->seq.s, seqLen, config->kSize, config->numHashes);
        if (nt == NULL)
        {
            fprintf(stderr, "could not initiate ntHash hasher\n");
            return -1;
        }

        // iterate over the k-mers and collect the hash(es)
        while (!nt->end)
        {
            if (bfAddPC(bf, nt->hashVector, config->numHashes))
            {
                fprintf(stderr, "could not add hashes to bloom filter\n");
                return -1;
            }
            ntIterate(nt);
        }

        // clean up
        ntDestroy(nt);

        // add bloom filter to map
        map_set(&bfMap, seq->name.s, *bf);
        bfDestroy(bf);
    }

    // finished with sequences
    slog(0, SLOG_INFO, "\t- processed %u sequences", seqCounter);
    slog(0, SLOG_INFO, "\t- added %u k-mers", kmerCounter);
    slog(0, SLOG_LIVE, "creating reference sketch database...");

    // add bloom filters to BIGSI
    if (bigsAdd(config->bigsi, bfMap, seqCounter) != 0)
        return -1;
    slog(0, SLOG_LIVE, "\t- added sequence bloom filters to a BIGSI");

    // index the BIGSI
    if (bigsIndex(config->bigsi) != 0)
        return -1;
    slog(0, SLOG_LIVE, "\t- indexed and saved to Berkeley DB");

    // save to disk and free
    if (bigsFlush(config->bigsi) != 0)
        return -1;
    config->bigsi = NULL;
    slog(0, SLOG_LIVE, "\t- written database to disk");

    // cleanup

    /*
    NEEd METHOd to WIPE BFs in MAP

    BIG LEAK....


    const char *key;
    map_iter_t iter = map_iter(&bfMap);

    while ((key = map_next(&bfMap, &iter)))
    {
        bfDestroy(map_get(&bfMap, key));
    }


*/

    map_deinit(&bfMap);
    kseq_destroy(seq);
    gzclose(fp);
    return 0;
}
