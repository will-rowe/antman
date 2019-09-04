#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define AM_DEFAULT_WATCH_DIR "/var/lib/MinKNOW/data/reads"

/*
    config_t is used to record the minimum information required by antman
*/
typedef struct config {
    char* configFile;
    char* watchDir;
    int pid;
    bool running;
} config_t;

/*
    function declarations
*/
config_t* initConfig();
void destroyConfig(config_t* config);
int writeConfig(config_t* config, char* configFile);
int loadConfig(config_t* config, char* configFile);

#endif