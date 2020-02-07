/*****************************************************************************
 * Package bigsi is a WIP library for Bit Sliced Genomic Signature Indexing.
 * 
 * To begin, add bloom filters to the BIGSI data structure with bigsAdd().
 * These bloom filters are provided via a map, where each key is a sequence
 * identifier and the value is a sequence bloom filter. bigsAdd() will check
 * the bloom filters and the corresponding sequence identifier, e.g. for
 * duplicate entries or incompatible settings, before assigning colours and
 * adding them to some temporary storage (tmpBitVectors and idChecker)
 * 
 * Once the desired number of bloom filters has been added (bigsAdd() can be
 * called multiple times), the BIGSI data structure can be indexed by calling
 * the bigsIndex() function.
 * 
 * bigsIndex() will iterate over all the bloom filters in the data structure,
 * creating a new bit vector for each bit index across all sequence IDs.
 * 
 * Once bigsIndex() has been run, it cannot be run again (at present) and all
 * input bloom filters will be deleted to save space.
 * 
 * 
 * Limitations:
 *      
 *          Currently, this BIGSI implementation is not thread safe.
 *          Filenames are hardcoded (directory can be provided though)
 * 
 */
#ifndef BIGSI_H
#define BIGSI_H

#include <db.h>
#include <limits.h>
#include <stdbool.h>

#include "bloomfilter.h"
#include "bitvector.h"
#include "3rd-party/map.h"

#define MAX_COLOURS INT_MAX       // the maximum number of colours a BIGSI can store
#define BERKELEY_DB_TYPE DB_BTREE // the type of Berkeley database to use
#define BIGSI_METADATA_FILENAME "bigsi-metadata.json"
#define BITVECTORS_DB_FILENAME "bigsi-bitvectors.bdb" // filename for the bitvectors
#define COLOURS_DB_FILENAME "bigsi-colours.bdb"       // filename for the colours

/*****************************************************************************
 * map_bloomfilter_t is a temporary structure for passing bloomfilters to a
 * BIGSI data structure to be indexed
 */
typedef map_t(bloomfilter_t) map_bloomfilter_t;

/*****************************************************************************
 * bigsi_t is the data structure for generating a BIGSI index
 */
typedef struct bigsi
{
    // persistent fields:
    int numBits;        // number of bits in each input bloom filter
    int numHashes;      // number of hash functions used to generate input bloom filter
    int colourIterator; // used to assign colours to input bloom filters
    bool indexed;       // bool to quickly check if indexing has been run (0==false, 1==true)

    // pre-indexing fields:
    bitvector_t **tmpBitVectors; // array of sequence bit vectors, indexed by their colour
    char **colourArray;          // array of sequence IDs, indexed by their colour
    map_int_t idChecker;         // map of sequence ID to colour to check if an ID has been seen before

    // post-indexing fields:
    DB *bitvectors_dbp;      // database containing the bit vectors
    DB *colours_dbp;         // database containing the sequence ids
    const char *dbDirectory; // directory containing the database files
    char *metadata_file;     // filename for metadata
    char *bitvectors_db;     // filename of the inventory database
    char *colours_db;        // filename of the colours database

} bigsi_t;

/*****************************************************************************
 * function prototypes
 */
bigsi_t *bigsInit(int numBits, int numHashes, const char *dbDir);
int bigsAdd(bigsi_t *bigsi, map_bloomfilter_t id2bf, int numEntries);
int bigsIndex(bigsi_t *bigsi);
int bigsQuery(bigsi_t *bigsi,uint64_t *hashValues, unsigned int len, bitvector_t *result);
int bigsLookupColour(bigsi_t *bigsi, int colour, char **result);
int bigsDestroy(bigsi_t *bigsi);
int bigsFlush(bigsi_t *bigsi);
bigsi_t *bigsLoad(const char *dbDir);

void setFilenames(bigsi_t *bigsi);
int initDBs(bigsi_t *bigsi, const char *, FILE *, u_int32_t openFlags);
int closeDBs(bigsi_t *bigsi);
int openDB(DB **, const char *, const char *, FILE *, u_int32_t openFlags);

#endif