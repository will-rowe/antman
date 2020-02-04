/*****************************************************************************
 * main.c is used to select subcommand and parse command line arguments
 * 
 * From main, we can launch the subcommands:
 *      - sketch
 * 
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/3rd-party/ketopt.h"
#include "../src/3rd-party/slog.h"

#include "subcommands.h"
#include "../src/config.h"
#include "../src/helpers.h"
#include "../src/daemonize.h"

/*****************************************************************************
 * printMainHelp displays the main program help.
 */
void printMainHelp(void)
{
    printf("\n"
           "===> %s | version %s <===\n\n"
           "Tag line to go here....\n\n"
           "Usage:\n"
           "\t%s sketch -h                               \t sketch a reference database\n"
           "\t%s set -h                                  \t update daemon settings\n"
           "\t%s info -h                                 \t print daemon information\n"
           "\t%s shrink -h                               \t launches the daemon\n"
           "\t%s stop -h                                 \t stops the daemon\n"
           "\n"
           "Options:\n"
           "\t -h                                        \t prints this help and exits\n"
           "\t -v                                        \t prints version number and exits\n",
           PROG_NAME, PROG_VERSION, PROG_NAME, PROG_NAME, PROG_NAME, PROG_NAME, PROG_NAME);
}

/*****************************************************************************
 * printsketchHelp displays the program help for the sketch subcommand.
 */
void printSketchHelp(void)
{
    printf("\n"
           "sketch a reference database\n\n"
           "Usage:\n"
           "\t%s sketch [options] <references.fasta>\n\n"
           "Sketching options:\n"
           "\t -k                                   \t k-mer size (default: %u)\n"
           "\t -m                                   \t est. max. number of k-mers per bloom filter (default: %u)\n"
           "\t -e                                   \t false positive error rate for bloom filters (default: %.4f)\n"
           "\n"
           "Database options:\n"
           "\t -o                                   \t directory to write database files to (default: %s)\n"
           "\n"
           "Miscellaneous options:\n"
           "\t -h                                   \t prints this help and exits\n",
           PROG_NAME, AM_DEFAULT_K_SIZE, AM_DEFAULT_BLOOM_MAX_EL, AM_DEFAULT_BLOOM_FP_RATE, AM_DEFAULT_DB_DIR);
}

/*****************************************************************************
 * printInfoHelp displays the program help for the info subcommand.
 */
void printInfoHelp(void)
{
    printf("\n"
           "prints information from the daemon\n\n"
           "Usage:\n"
           "\t%s info [options]\n\n"
           "Daemon options:\n"
           "\t -p                                   \t print PID only\n"
           "\n"
           "Miscellaneous options:\n"
           "\t -h                                   \t prints this help and exits\n",
           PROG_NAME);
}

/*****************************************************************************
 * printSetHelp displays the program help for the set subcommand.
 */
void printSetHelp(void)
{
    printf("\n"
           "update daemon settings\n\n"
           "Usage:\n"
           "\t%s set [options]\n\n"
           "Daemon options:\n"
           "\t -l path/to/log.file                  \t set log file\n"
           "\t -w path/to/dir                       \t set watch directory\n"
           "\n"
           "Miscellaneous options:\n"
           "\t -h                                   \t prints this help and exits\n",
           PROG_NAME);
}

/*****************************************************************************
 * printShrinkHelp displays the program help for the shrink subcommand.
 */
void printShrinkHelp(void)
{
    printf("\n"
           "launch the daemon\n\n"
           "Usage:\n"
           "\t%s shrink [options]\n\n"
           "Daemon options:\n"
           "\n"
           "Miscellaneous options:\n"
           "\t -h                                   \t prints this help and exits\n",
           PROG_NAME);
}

/*****************************************************************************
 * printStopHelp displays the program help for the stop subcommand.
 */
void printStopHelp(void)
{
    printf("\n"
           "stop the daemon\n\n"
           "Usage:\n"
           "\t%s stop [options]\n\n"
           "Daemon options:\n"
           "\n"
           "Miscellaneous options:\n"
           "\t -h                                   \t prints this help and exits\n",
           PROG_NAME);
}

/*****************************************************************************
 * configLoader is helper function to load the config.
 * 
 * returns:
 *      a loaded config
 * 
 * note:
 *      a config will be created if it does not exist
 *      the user must check and free the returned config
 */
config_t *configLoader(void)
{
    // init an empty config struct
    config_t *config = initConfig();
    if (config == NULL)
    {
        fprintf(stderr, "error: failed to init a config\n\n");
        return NULL;
    }

    // check if a config file exists and is accessible
    if (access(CONFIG_LOCATION, R_OK | W_OK) == -1)
    {
        // touch the config filepath
        if (checkFilePath(CONFIG_LOCATION) != 0)
        {
            fprintf(stderr, "error: could not touch config file (%s)\n\n", CONFIG_LOCATION);
        }

        // write a new config
        if (writeConfig(config, CONFIG_LOCATION) != 0)
        {
            destroyConfig(config);
            fprintf(stderr, "\nerror: failed to create a config file at %s\n\n", CONFIG_LOCATION);
            return NULL;
        }
    }

    // load the config file into the struct
    if (loadConfig(config, CONFIG_LOCATION) != 0)
    {
        destroyConfig(config);
        fprintf(stderr, "\nerror: failed to the load config file\n\n");
        return NULL;
    }
    return config;
}

/*****************************************************************************
 * greet is helper function to check the settings and greet the user.
 * 
 * arguments:
 *      config                     - a loaded config
 *      checkDB                    - flag to check the reference database
 *      cmd                        - the subcommand being run
 * 
 * returns:
 *      0 on success, -1 on error
 */
int greet(config_t *config, bool checkDB, char *cmd)
{
    // check the config
    if (checkConfig(config, checkDB) != 0)
    {
        return -1;
    }

    // banner
    printf("=======================================================\n");
    printf("%s (version: %s)\n", PROG_NAME, PROG_VERSION);
    printf("=======================================================\n");

    // start the log
    slog_init(config->currentLogFile, NULL, 1, 0);
    slog(0, SLOG_LIVE, "reading config...");
    slog(0, SLOG_INFO, "\t- config location: %s", CONFIG_LOCATION);
    slog(0, SLOG_INFO, "\t- config created on: %s", config->created);
    slog(0, SLOG_INFO, "\t- config last modified on: %s", config->modified);
    slog(0, SLOG_INFO, "\t- current log file: %s", config->currentLogFile);
    slog(0, SLOG_INFO, "\t- watch directory: %s", config->watchDir);
    slog(0, SLOG_INFO, "\t- white list: %s", config->white_list);
    slog(0, SLOG_INFO, "\t- k-mer size: %d", config->kSize);
    slog(0, SLOG_INFO, "\t- max number of k-mers per bloom filter: %d", config->maxElements);
    slog(0, SLOG_INFO, "\t- bloom filter false positive rate: %f", config->fpRate);
    slog(0, SLOG_LIVE, "starting %s subcommand...", cmd);
    return 0;
}

/*****************************************************************************
 * main is the entry point.
 */
int main(int argc, char *argv[])
{
    ketopt_t om = KETOPT_INIT, os = KETOPT_INIT;
    int i, c;
    config_t *config;

    // check for help and version requests, anything else will be ignored if there is no subcommand
    while ((c = ketopt(&om, argc, argv, 0, "hv", 0)) >= 0)
    {

        // prints help and exits
        if (c == 'h')
        {
            printMainHelp();
            return 0;
        }

        // prints version and exits
        if (c == 'v')
        {
            printf("%s\n", PROG_VERSION);
            return 0;
        }
    }

    // check a subcommand was provided
    if (om.ind == argc)
    {
        fprintf(stderr, "error: no subcommand provided\n\n");
        printMainHelp();
        return -1;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    /* - info - */
    if (strcmp(argv[om.ind], "info") == 0)
    {
        bool pidOnly = false;

        // check the arguments for info
        while ((c = ketopt(&os, argc - om.ind, argv + om.ind, 1, "hp", 0)) >= 0)
        {
            if (c == 'h')
            {
                printInfoHelp();
                return 0;
            }
            if (c == 'p')
            {
                pidOnly = true;
            }
        }

        // load the config and run the info function
        if ((config = configLoader()) == NULL)
            return -1;
        int retVal = info(config, pidOnly);
        destroyConfig(config);
        return retVal;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    /* - set - */
    if (strcmp(argv[om.ind], "set") == 0)
    {

        // load the config
        if ((config = configLoader()) == NULL)
            return -1;

        // TODO: for now, only run sketch when the daemon is not already running - do we want to handle this differently?
        if (checkPID(config) >= 0)
        {
            fprintf(stderr, "error: daemon is already running on PID %u\n\n", config->pid);
            destroyConfig(config);
            return -1;
        }

        // check the arguments for set and update the in-memory config
        bool optErr = false, optCheck = false;
        while ((c = ketopt(&os, argc - om.ind, argv + om.ind, 1, "hw:l:", 0)) >= 0)
        {
            optCheck = true;
            if (c == 'h')
            {
                printSetHelp();
                destroyConfig(config);
                return 0;
            }

            // set log file
            if (c == 'l')
            {
                if (setConfigField(config, 0, os.arg) != 0)
                    optErr = true;
            }

            // set watch directory
            if (c == 'w')
            {
                if (setConfigField(config, 1, os.arg) != 0)
                    optErr = true;
            }

            // TODO: set white list
            //
            //
        }

        // check for any errors in the subcommand arguments
        if (optErr)
        {
            destroyConfig(config);
            return -1;
        }
        if (!optCheck)
        {
            fprintf(stderr, "error: no options passed to set, nothing to do\n\n");
            printSetHelp();
            destroyConfig(config);
            return -1;
        }

        // write the in-memory config to disk
        if (writeConfig(config, config->filename) != 0)
        {
            fprintf(stderr, "could not update the config file\n");
            destroyConfig(config);
            return -1;
        }
        destroyConfig(config);
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    /* - sketch - */
    if (strcmp(argv[om.ind], "sketch") == 0)
    {

        // load the config
        if ((config = configLoader()) == NULL)
            return -1;

        // TODO: for now, only run sketch when the daemon is not already running - do we want to handle this differently?
        if (checkPID(config) >= 0)
        {
            fprintf(stderr, "error: daemon is already running on PID %u\n\n", config->pid);
            destroyConfig(config);
            return -1;
        }

        // check the arguments for sketch and update the in-memory config
        bool optErr = false;
        while ((c = ketopt(&os, argc - om.ind, argv + om.ind, 1, "hk:m:e:o:", 0)) >= 0)
        {
            if (c == 'h')
            {
                printSketchHelp();
                destroyConfig(config);
                return 0;
            }
            if (c == 'k')
            {
                if (setConfigField(config, 3, os.arg) != 0)
                    optErr = true;
            }
            if (c == 'm')
            {
                if (setConfigField(config, 4, os.arg) != 0)
                    optErr = true;
            }
            if (c == 'e')
            {
                if (setConfigField(config, 5, os.arg) != 0)
                    optErr = true;
            }
            if (c == 'o')
            {
                if (setConfigField(config, 6, os.arg) != 0)
                    optErr = true;
            }
        }

        // make sure there is an output directory for the database
        if (!config->dbDir)
        {
            if (setConfigField(config, 6, AM_DEFAULT_DB_DIR) != 0)
                optErr = true;
        }

        // check for any errors in the subcommand arguments
        if (optErr)
        {
            destroyConfig(config);
            return -1;
        }

        // greet the user and check the config
        if (greet(config, false, argv[om.ind]))
        {
            destroyConfig(config);
            return -1;
        }

        // set up the requested database
        // TODO: only BIGSI hardcoded for now
        if (setupDB(config, 0))
        {
            destroyConfig(config);
            return -1;
        }
        slog(0, SLOG_INFO, "\t- number of hashes per bloom filter: %llu", config->numHashes);
        slog(0, SLOG_INFO, "\t- number of bits used per bloom filter: %llu", config->numBits);
        slog(0, SLOG_INFO, "\t- writing BIGSI database at: %s", config->dbDir);

        // run sketch with either the positional arguments or STDIN
        slog(0, SLOG_LIVE, "collecting sequences...\n");
        bool useSTDIN = true;
        for (i = os.ind + om.ind; i < argc; ++i)
        {
            useSTDIN = false;
            if (sketch(config, argv[i]))
            {
                destroyConfig(config);
                return -1;
            }
        }
        if (useSTDIN)
        {
            if (sketch(config, NULL))
            {
                destroyConfig(config);
                return -1;
            }
        }

        // finish up by writing the run info to the config
        if (writeConfig(config, config->filename) != 0)
        {
            fprintf(stderr, "could not update the config file\n");
            destroyConfig(config);
            return -1;
        }
        slog(0, SLOG_LIVE, "finished");
        destroyConfig(config);
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    /* - shrink - */
    if (strcmp(argv[om.ind], "shrink") == 0)
    {

        // check the arguments for shrink
        while ((c = ketopt(&os, argc - om.ind, argv + om.ind, 1, "h", 0)) >= 0)
        {
            if (c == 'h')
            {
                printShrinkHelp();
                return 0;
            }
        }

        // load the config
        if ((config = configLoader()) == NULL)
            return -1;

        // check not already running
        if (checkPID(config) >= 0)
        {
            fprintf(stderr, "error: daemon is already running on PID %u\n\n", config->pid);
            destroyConfig(config);
            return -1;
        }

        // set up a default log if needed
        if (config->currentLogFile == NULL)
        {
            if (createLogFile(config) != 0)
            {
                destroyConfig(config);
                return -1;
            }
        }

        // greet the user and check the config
        if (greet(config, true, argv[om.ind]))
        {
            destroyConfig(config);
            return -1;
        }

        // load the database
        if (loadDB(config, 0))
        {
            destroyConfig(config);
            return -1;
        }
        slog(0, SLOG_INFO, "\t- reference database location: %s", config->dbDir);
        slog(0, SLOG_INFO, "\t- number of hashes functions used in BIGSI: %llu", config->numHashes);
        slog(0, SLOG_INFO, "\t- number of rows in BIGSI: %llu", config->numBits);
        slog(0, SLOG_INFO, "\t- number of colours in BIGSI: %u", config->bigsi->colourIterator);

        // run the shrink function
        shrink(config);

        // TODO: I'm keeping all of this in the main function for now
        // wargs seems unecessary when I have an in-memory config
        // set up the watch directory
        slog(0, SLOG_INFO, "setting up the directory watcher...");
        watcherArgs_t *wargs = malloc(sizeof(watcherArgs_t));
        if (wargs == NULL)
        {
            slog(0, SLOG_ERROR, "could not allocate the watcher arguments");
            destroyConfig(config);
            return 1;
        }
        wargs->kSize = config->kSize;
        wargs->fp_rate = config->fpRate;

        // start the daemon
        slog(0, SLOG_INFO, "starting the daemon...");
        if (startDaemon(config, wargs) != 0)
        {
            free(wargs);
            destroyConfig(config);
            return -1;
        }

        // daemon has been killed
        // TODO: have shrink destroy the config when done?
        free(wargs);
        destroyConfig(config);
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    /* - stop - */
    if (strcmp(argv[om.ind], "stop") == 0)
    {

        // check the arguments for stop
        while ((c = ketopt(&os, argc - om.ind, argv + om.ind, 1, "h", 0)) >= 0)
        {
            if (c == 'h')
            {
                printStopHelp();
                return 0;
            }
        }

        // load the config and run the stop function
        if ((config = configLoader()) == NULL)
            return -1;
        if (checkPID(config) < 0)
        {
            fprintf(stderr, "error: can't stop a daemon that is not running\n\n");
            return -1;
        }
        int retVal = stop(config);
        destroyConfig(config);
        return retVal;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // error if we haven't recognised the subcommand
    else
    {
        fprintf(stderr, "error: unrecognised subcommand (%s)\n\n", argv[om.ind]);
        printMainHelp();
        return -1;
    }
}
