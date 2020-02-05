/*****************************************************************************
 * Package config is used to orchestrate the deamon, serialise and load
 * runtime information.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#include "bigsi.h"

// default and max defines
#define AM_DEFAULT_LOG "antman.log"
#define AM_DEFAULT_DB_DIR "/tmp"
#define AM_DEFAULT_K_SIZE 21
#define AM_DEFAULT_SKETCH_SIZE 128
#define AM_DEFAULT_BLOOM_FP_RATE 0.001
#define AM_DEFAULT_BLOOM_MAX_EL 100000

#define AM_MAX_K_SIZE 31
#define AM_MAX_BLOOM_FP_RATE 0.1
#define AM_MAX_BLOOM_MAX_EL 1000000

/*****************************************************************************
 * config_t is used to record the minimum information required by antman
 */
typedef struct config
{
    char *filename;
    char *created;
    char *modified;
    char *currentLogFile;
    char *watchDir;
    char *dbDir;
    char *white_list;
    int pid;
    int kSize;
    int maxElements;
    double fpRate;

    // calculated for bloom filters / BIGSI:
    uint64_t numBits;   // number of bits in each input bloom filter
    uint64_t numHashes; // number of hash functions used to generate input bloom filter

    // in-memory fields only:
    bigsi_t *bigsi; // database struct used when config in memory
    int numThreads;
} config_t;

/*****************************************************************************
 * function prototypes
 */
config_t *configInit();
void configDestroy(config_t *config);
int configWrite(config_t *config, char *configFile);
int configLoad(config_t *config, char *configFile);
int configCheck(config_t *config, bool testDB);
int setConfigField(config_t *config, int field, char *text);
int createLogFile(config_t *config);
int setupDB(config_t *config, int dbType);
int loadDB(config_t *config, int dbType);

#endif