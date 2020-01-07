#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "frozen.h"

// initConfig
config_t *initConfig()
{
    config_t *c;
    if ((c = malloc(sizeof *c)) != NULL)
    {
        c->filename = NULL;
        c->created = NULL;
        c->modified = NULL;
        c->current_log_file = NULL;
        c->watch_directory = NULL;
        c->white_list = NULL;
        c->pid = -1;
        c->k_size = AM_DEFAULT_K_SIZE;
        c->sketch_size = AM_DEFAULT_SKETCH_SIZE;
        c->bloom_fp_rate = AM_DEFAULT_BLOOM_FP_RATE;
        c->bloom_max_elements = AM_DEFAULT_BLOOM_MAX_EL;
        c->bloom_filter = NULL;
    }
    return c;
}

// destroyConfig
void destroyConfig(config_t *config)
{
    free(config->filename);
    free(config->created);
    free(config->modified);
    free(config->current_log_file);
    free(config->watch_directory);
    free(config->white_list);
    free(config);
    config = NULL;
}

// writeConfig
int writeConfig(config_t *config, char *configFile)
{

    // make sure the config is populated with something
    if (config == 0)
        return 1;

    // if this is a new filepath, update with config with the filepath we are writing to
    if (config->filename != configFile)
    {
        free(config->filename);
        config->filename = strdup(configFile);
    }

    // update the created (if new) and the modified date
    SlogDate date;
    slog_get_date(&date);
    char *timeStamp = malloc(18 * sizeof(char));
    int ret = sprintf(timeStamp, "%d-%d-%d:%d-%d", date.year, date.mon, date.day, date.hour, date.min);
    if (ret > 18)
    {
        fprintf(stderr, "failed to format time stamp\n");
        return 1;
    }
    if (config->created == NULL)
    {
        config->created = strdup(timeStamp);
    }
    config->modified = timeStamp;

    // write it to file
    ret = json_fprintf(configFile, "{ filename: %Q, created: %Q, modified: %Q, current_log_file: %Q, watch_directory: %Q, white_list: %Q, pid: %d, k_size: %d, sketch_size: %d, bloom_fp_rate: %f, bloom_max_elements: %d }",
                       config->filename,
                       config->created,
                       config->modified,
                       config->current_log_file,
                       config->watch_directory,
                       config->white_list,
                       config->pid,
                       config->k_size,
                       config->sketch_size,
                       config->bloom_fp_rate,
                       config->bloom_max_elements);
    if (ret < 0)
    {
        fprintf(stderr, "failed to write config to disk (%d)\n", ret);
        return 1;
    }

    // prettify the json
    json_prettify_file(configFile);
    return 0;
}

// loadConfig
int loadConfig(config_t *config, char *configFile)
{

    // set up some string vars to capture the json content
    char *filename = NULL;
    char *created = NULL;
    char *modified = NULL;
    char *current_log_file = NULL;
    char *watch_directory = DEFAULT_WATCH_DIR;
    char *white_list = NULL;

    // read the file into a buffer
    char *content = json_fread(configFile);

    // scan the file content and populate the tmp config
    int status = json_scanf(content, strlen(content), "{ filename: %Q, created: %Q, modified: %Q, current_log_file: %Q, watch_directory: %Q, white_list: %Q, pid: %d, k_size: %d, sketch_size: %d, bloom_fp_rate: %f, bloom_max_elements: %d }",
                            &filename,
                            &created,
                            &modified,
                            &current_log_file,
                            &watch_directory,
                            &white_list,
                            &config->pid,
                            &config->k_size,
                            &config->sketch_size,
                            &config->bloom_fp_rate,
                            &config->bloom_max_elements);

    // free the buffer
    free(content);

    // check for error in json scan (-1 == error, 0 == no elements found, >0 == elements parsed)
    if (status < 1)
    {
        return 1;
    }

    // copy over the string content to our config and free the holders
    if (filename)
    {
        config->filename = strdup(filename);
        free(filename);
    }
    if (created)
    {
        config->created = strdup(created);
        free(created);
    }
    if (modified)
    {
        config->modified = strdup(modified);
        free(modified);
    }
    if (current_log_file)
    {
        config->current_log_file = strdup(current_log_file);
        free(current_log_file);
    }
    if (watch_directory)
    {
        config->watch_directory = strdup(watch_directory);
        free(watch_directory);
    }
    if (white_list)
    {
        config->white_list = strdup(white_list);
        free(white_list);
    }
    return 0;
}
