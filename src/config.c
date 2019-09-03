#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "config.h"
#include "frozen.h"

// initConfig
Config* initConfig() {
    Config* c;
    if ((c = malloc(sizeof *c)) != NULL) {
        c->configFile = "";
        c->watchDir = AM_DEFAULT_WATCH_DIR;
        c->pid = -1;
        c->running = false;
    }
    return c;
}

// destroyConfig
void destroyConfig(Config *config) {
    free(config);
    config = NULL;
}

// writeConfig
int writeConfig(Config* config, char* configFile) {

    // make sure the config is populated with something
    if (config == 0) return 1;

    // update with config with the filepath we are writing to
    config->configFile = configFile;

    // write it to file
    json_fprintf(configFile, "{ configFile: %Q, watchDirectory: %Q, pid: %d, running: %B }", config->configFile, config->watchDir, config->pid, config->running);

    // prettify the json
    json_prettify_file(configFile);
    return 0;
}

// loadConfig
int loadConfig(Config* config, char* configFile) {

    // create a stack allocated tmp config
    Config c = { .pid = -1, .watchDir = NULL };

    // read the file into a buffer
    char* content = json_fread(configFile);

    // scan the file content and populate the tmp config
    int status = json_scanf(content, strlen(content), "{ configFile: %Q, watchDirectory: %Q, pid: %d, running: %B }", &c.configFile, &c.watchDir, &c.pid, &c.running);

    // check for error in json scan (-1 == error, 0 == no elements found, >0 == elements parsed)
    if (status < 1) {
        return 1;
    }

    // copy the config over to the heap
    config->configFile = c.configFile;
    config->watchDir = c.watchDir;
    config->pid = c.pid;
    config->running = c.running;
    return 0;
}
