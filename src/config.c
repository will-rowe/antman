#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "frozen.h"

// initConfig
config_t* initConfig() {
    config_t* c;
    if ((c = malloc(sizeof *c)) != NULL) {
        c->filename = "";
        c->created = NULL;
        c->modified = "";
        c->current_log_file = "";
        c->watch_directory = DEFAULT_WATCH_DIR;
        c->pid = -1;
        c->k_size = AM_DEFAULT_K_SIZE;
        c->sketch_size = AM_DEFAULT_SKETCH_SIZE;
        c->bloom_fp_rate = AM_DEFAULT_BLOOM_FP_RATE;
        c->bloom_max_elements = AM_DEFAULT_BLOOM_MAX_EL;
    }
    return c;
}

// destroyConfig
void destroyConfig(config_t* config) {
    free(config);
    config = NULL;
}

// writeConfig
int writeConfig(config_t* config, char* configFile) {

    // make sure the config is populated with something
    if (config == 0) return 1;

    // update with config with the filepath we are writing to
    config->filename = configFile;

    // update the created (if new) and the modified date
    SlogDate date;
    slog_get_date(&date);
    char timeStamp[18];
    int ret = snprintf(timeStamp, sizeof(timeStamp), "%d-%d-%d:%d%d", date.year, date.mon, date.day, date.hour, date.min);
    if (ret > 18) {
        fprintf(stderr, "failed to format time stamp\n");
        return 1;
    }
    if (config->created == NULL) {
        config->created = timeStamp;
    }
    config->modified = timeStamp;

    // write it to file
    ret = json_fprintf(configFile, "{ filename: %Q, created: %Q, modified: %Q, current_log_file: %Q, watch_directory: %Q, pid: %d, k_size: %d, sketch_size: %d, bloom_fp_rate: %f, bloom_max_elements: %d }",
    config->filename,
    config->created,
    config->modified,
    config->current_log_file,
    config->watch_directory,
    config->pid,
    config->k_size,
    config->sketch_size,
    config->bloom_fp_rate,
    config->bloom_max_elements   
    );
    if (ret < 0) {
        fprintf(stderr, "failed to write config to disk (%d)\n", ret);
        return 1;
    }

    // prettify the json
    json_prettify_file(configFile);
    return 0;
}

// loadConfig
int loadConfig(config_t* config, char* configFile) {

    // create a stack allocated tmp config
    config_t c = { .pid = -1, .watch_directory = NULL };

    // read the file into a buffer
    char* content = json_fread(configFile);

    // scan the file content and populate the tmp config
    int status = json_scanf(content, strlen(content), "{ filename: %Q, created: %Q, modified: %Q, current_log_file: %Q, watch_directory: %Q, pid: %d, k_size: %d, sketch_size: %d, bloom_fp_rate: %f, bloom_max_elements: %d }",
    &c.filename,
    &c.created,
    &c.modified,
    &c.current_log_file,
    &c.watch_directory,
    &c.pid,
    &c.k_size,
    &c.sketch_size,
    &c.bloom_fp_rate,
    &c.bloom_max_elements 
    );

    // check for error in json scan (-1 == error, 0 == no elements found, >0 == elements parsed)
    if (status < 1) {
        return 1;
    }

    // copy the config over to the heap
    config->filename = c.filename;
    config->created = c.created;
    config->modified = c.modified;
    config->current_log_file = c.current_log_file;
    config->watch_directory = c.watch_directory;
    config->pid = c.pid;
    config->k_size = c.k_size;
    config->sketch_size = c.sketch_size;
    config->bloom_fp_rate = c.bloom_fp_rate;
    config->bloom_max_elements = c.bloom_max_elements;
    return 0;
}
