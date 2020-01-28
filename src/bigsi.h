/*****************************************************************************
 * Package bigsi is a WIP library for Bit Sliced Genomic Signature Indexing.
 * 
 * To begin, add bloom filters to the BIGSI data structure with bigsAdd().
 * These bloom filters are provided via a map, where each key is a sequence
 * identifier and the value is a sequence bloom filter. bigsAdd() will check
 * the bloom filters and the corresponding sequence identifier, e.g. for
 * duplicate entries or incompatible settings, before assigning colours and
 * adding them to some temporary storage (bvArray and id2colour)
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
 */
#ifndef BIGSI_H
#define BIGSI_H

#include <db.h>
#include <limits.h>

#include "bloomfilter.h"
#include "bitvector.h"
#include "map.h"

#define MAX_COLOURS INT_MAX                           // the maximum number of colours a BIGSI can store
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
    bitvector_t **bvArray; // array of sequence bit vectors, indexed by their colour -> used prior to indexing
    map_int_t id2colour;   // map of sequence ID to colour -> used prior to indexing
    char **colourArray;    // array of sequence IDs, indexed by their colour
    bitvector_t **index;   // array of BIGSI bit vectors, indexed by bit index of input bloom filters
    int numBits;           // number of bits in each input bloom filter
    int numHashes;         // number of hash functions used to generate input bloom filter
    int colourIterator;    // used to assign colours to input bloom filters
} bigsi_t;

/*****************************************************************************
 * BIGSI_DB_t is the BIGSI index
 */
typedef struct bigsi_db
{
    DB *bitvectors_dbp;       /* Database containing the bit vectors */
    DB *colours_dbp;          /* Database containing the sequence ids */
    const char *db_home_dir;  /* Directory containing the database files */
    char *bitvectors_db_name; /* Name of the inventory database */
    char *colours_db_name;    /* Name of the colours database */
} BIGSI_DB_t;

/*****************************************************************************
 * function prototypes
 */
bigsi_t *bigsInit(int numBits, int numHashes);
int bigsAdd(bigsi_t *bigsi, map_bloomfilter_t id2bf, int numEntries);
int bigsIndex(bigsi_t *bigsi);
int bigsQuery(bigsi_t *bigsi, const void *buffer, int len, bitvector_t *result);
int bigsDestroy(bigsi_t *bigsi);
int bigsDump(bigsi_t *bigsi, const char *filepath);
bigsi_t *bigsLoad(const char *filepath);

int databases_setup(BIGSI_DB_t *, const char *, FILE *, u_int32_t openFlags);
int databases_close(BIGSI_DB_t *);
void initialize_stockdbs(BIGSI_DB_t *, const char *filepath);
int open_database(DB **, const char *, const char *, FILE *, u_int32_t openFlags);
void set_db_filenames(BIGSI_DB_t *my_stock);

#endif