#ifndef CONFIG_H
#define CONFIG_H

#include "bloomfilter.h"
#include "3rd-party/slog.h"

// TODO: these are temp defines until the CLI is finished
#define AM_DEFAULT_K_SIZE 7
#define AM_DEFAULT_SKETCH_SIZE 128
#define AM_DEFAULT_BLOOM_FP_RATE 0.001
#define AM_DEFAULT_BLOOM_MAX_EL 100000

/*
    config_t is used to record the minimum information required by antman
*/
typedef struct config
{
    char *filename;
    char *created;
    char *modified;
    char *current_log_file;
    char *watch_directory;
    char *white_list;
    int pid;
    int k_size;
    int sketch_size;
    double bloom_fp_rate;
    int bloom_max_elements;
    bloomfilter_t *bloom_filter;
} config_t;

/*
    function prototypes
*/
config_t *initConfig();
void destroyConfig(config_t *config);
int writeConfig(config_t *config, char *configFile);
int loadConfig(config_t *config, char *configFile);
void slog_get_date(SlogDate *pDate);

#endif