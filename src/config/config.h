#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define DD_DEFAULT_WATCH_DIR "/var/lib/MinKNOW/data/reads"

// Config is the struct containing config information
typedef struct Config {
    char *configFile;
    char *watchDir;
    int pid;
    bool running;
} Config;

/*
    function declarations
*/
Config* initConfig();
void destroyConfig(Config* config);
int writeConfig(Config* config, char* configFile);
int loadConfig(Config* config, char* configFile);

#endif