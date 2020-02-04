#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "3rd-party/frozen.h"

#include "bloomfilter.h"
#include "config.h"
#include "helpers.h"

/*****************************************************************************
 * initConfig.
 */
config_t *initConfig()
{
    config_t *c;
    if ((c = malloc(sizeof *c)) != NULL)
    {
        c->filename = NULL;
        c->created = NULL;
        c->modified = NULL;
        c->currentLogFile = NULL;
        c->watchDir = NULL;
        c->dbDir = NULL;
        c->white_list = NULL;
        c->pid = -1;
        c->kSize = AM_DEFAULT_K_SIZE;
        c->maxElements = AM_DEFAULT_BLOOM_MAX_EL;
        c->fpRate = AM_DEFAULT_BLOOM_FP_RATE;
        c->numBits = 0;
        c->numHashes = 0;
        c->bigsi = NULL;
    }
    return c;
}

/*****************************************************************************
 * destroyConfig.
 */
void destroyConfig(config_t *config)
{
    free(config->filename);
    free(config->created);
    free(config->modified);
    free(config->currentLogFile);
    free(config->watchDir);
    free(config->dbDir);
    free(config->white_list);
    if (config->bigsi)
        bigsDestroy(config->bigsi); // TODO: should check for errors here
    free(config);
    config = NULL;
}

/*****************************************************************************
 * writeConfig will write a json config from an initiated config_t struct.
 */
int writeConfig(config_t *config, char *configFile)
{
    if (config == NULL)
    {
        return -1;
    }

    // if this is a new filepath, update with config with the filepath we are writing to
    if (config->filename != configFile)
    {
        free(config->filename);
        config->filename = strdup(configFile);
    }

    // update the created (if new) and the modified date
    char *timeStamp;
    if (getTimeStamp(&timeStamp) != 0)
        return -1;
    if (config->created == NULL)
    {
        config->created = strdup(timeStamp);
    }
    config->modified = strdup(timeStamp);
    free(timeStamp);

    // write it to file
    int ret;
    ret = json_fprintf(configFile, "{ filename: %Q, created: %Q, modified: %Q, currentLogFile: %Q, watchDir: %Q, dbDir: %Q, white_list: %Q, pid: %d, kSize: %d, maxElements: %d, fpRate: %f, numBits: %d, numHashes: %d }",
                       config->filename,
                       config->created,
                       config->modified,
                       config->currentLogFile,
                       config->watchDir,
                       config->dbDir,
                       config->white_list,
                       config->pid,
                       config->kSize,
                       config->maxElements,
                       config->fpRate,
                       config->numBits,
                       config->numHashes);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to write config to disk (%d)\n", ret);
        return -1;
    }

    // prettify the json
    json_prettify_file(configFile);
    return 0;
}

/*****************************************************************************
 * loadConfig will read a json config into an initiated config_t struct.
 */
int loadConfig(config_t *config, char *configFile)
{
    if (config == NULL)
    {
        return -1;
    }

    // read the file into a buffer
    char *content = json_fread(configFile);

    // scan the file content and populate the tmp config
    int status = json_scanf(content, strlen(content), "{ filename: %Q, created: %Q, modified: %Q, currentLogFile: %Q, watchDir: %Q, dbDir: %Q, white_list: %Q, pid: %d, kSize: %d, maxElements: %d, fpRate: %f, numBits: %d, numHashes: %d }",
                            &config->filename,
                            &config->created,
                            &config->modified,
                            &config->currentLogFile,
                            &config->watchDir,
                            &config->dbDir,
                            &config->white_list,
                            &config->pid,
                            &config->kSize,
                            &config->maxElements,
                            &config->fpRate,
                            &config->numBits,
                            &config->numHashes);

    // free the buffer
    free(content);

    // check for error in json scan (-1 == error, 0 == no elements found, >0 == elements parsed)
    if (status < 1)
    {
        return -1;
    }
    return 0;
}

/*****************************************************************************
 * checkConfig checks required fields.
 * 
 * arguments:
 *      config                  - the config to check
 *      testDB                  - if true, the reference database will be checked
 * 
 * returns:
 *      0 on success, -1 on error
 */
int checkConfig(config_t *config, bool testDB)
{
    if (config == NULL)
    {
        return -1;
    }
    // check for fields
    if (!config->filename)
    {
        fprintf(stderr, "error: no filename found for config\n");
        return -1;
    }
    if (!config->watchDir)
    {
        fprintf(stderr, "error: no watch directory listed in the config\n");
        return -1;
    }
    if (!config->currentLogFile)
    {
        fprintf(stderr, "error: no log file listed in the config, creating one now...\n");
        if (createLogFile(config))
            return -1;
    }
    if (testDB)
    {
        if (!config->dbDir)
        {
            fprintf(stderr, "error: no reference database listed in the config\n");
            return -1;
        }
    }

    // check for existence
    if (checkDirectory(config->watchDir, false) == -1)
    {
        fprintf(stderr, "error: watch directory does not exist (%s)\n", config->watchDir);
        return -1;
    }
    if (access(config->currentLogFile, R_OK | W_OK) == -1)
    {
        fprintf(stdout, "registered log file does not exist: %s\n", config->currentLogFile);
        fprintf(stdout, "replacing with a new log file...\n");
        free(config->currentLogFile);
        config->currentLogFile = NULL;
        if (createLogFile(config))
            return -1;
        fprintf(stdout, "new log file created: %s\n", config->currentLogFile);
    }
    if (testDB)
    {
        if (checkDirectory(config->dbDir, false) == -1)
        {
            fprintf(stderr, "error: config has incorrect reference database location\n");
            return -1;
        }
    }
    return 0;
}

/*****************************************************************************
 * setConfigField will set a field in the config struct.
 * 
 * available fields:
 *      0 - log file (will be created if non existant)
 *      1 - watch directory (will FAIL if non existant)
 *      2 - white list
 *      3 - kSize
 *      4 - maxElements
 *      5 - fpRate
 *      6 - dbDir
 * 
 * note:
 *      the caller must free the supplied text
 *      it's the caller's job to write the config to disk
 */
int setConfigField(config_t *config, int field, char *text)
{
    if (text == NULL)
    {
        fprintf(stderr, "error: no text sent to setConfigField\n");
        return -1;
    }

    // get the field
    switch (field)
    {
    case 0:
        if (checkFilePath(text))
            return -1;
        if (config->currentLogFile != NULL)
        {
            free(config->currentLogFile);
        }
        config->currentLogFile = realpath(text, NULL);
        if (config->currentLogFile == NULL)
        {
            fprintf(stderr, "error: could not get absolute path to %s\n", text);
            return -1;
        }
        break;

    case 1:
        if (checkDirectory(text, false))
            return -1;
        if (config->watchDir != NULL)
        {
            free(config->watchDir);
        }
        config->watchDir = realpath(text, NULL);
        if (config->watchDir == NULL)
        {
            fprintf(stderr, "error: could not get absolute path to %s\n", text);
            return -1;
        }
        break;

    case 2:
        fprintf(stderr, "error: todo\n");
        break;

    case 3:
        config->kSize = atoi(text);
        if (config->kSize > AM_MAX_K_SIZE)
        {
            fprintf(stderr, "error: supplied k-mer exceeds limit (%u vs %u)\n", config->kSize, AM_MAX_K_SIZE);
            return -1;
        }
        break;

    case 4:
        config->maxElements = atoi(text);
        if (config->maxElements > AM_MAX_BLOOM_MAX_EL)
        {
            fprintf(stderr, "error: supplied max number of k-mers exceeds limit (%u vs %u)\n", config->maxElements, AM_MAX_BLOOM_MAX_EL);
            return -1;
        }
        break;

    case 5:
        config->fpRate = atof(text);
        if (config->fpRate > AM_MAX_BLOOM_FP_RATE)
        {
            fprintf(stderr, "error: supplied max number of k-mers exceeds limit (%f vs %f)\n", config->fpRate, AM_MAX_BLOOM_FP_RATE);
            return -1;
        }
        break;

    case 6:
        if (checkDirectory(text, true))
            return -1;
        if (config->dbDir != NULL)
        {
            free(config->dbDir);
        }
        config->dbDir = realpath(text, NULL);
        if (config->dbDir == NULL)
        {
            fprintf(stderr, "error: could not get absolute path to %s\n", text);
            return -1;
        }
        break;

    default:
        fprintf(stderr, "error: unknown field passed to setConfigField\n");
        return -1;
    }
    return 0;
}

/*****************************************************************************
 * createLogFile will create a default log file and store it's location in
 * the config on disk.
 * 
 * TODO: the logging library was giving me an error (Illegal instruction: 4)
 *          when the log name was too long I think - have removed the timestamp
 *          stuff for now to get logging working on test system. 
 */
int createLogFile(config_t *config)
{
    if (config == NULL)
    {
        return -1;
    }
    if (config->currentLogFile != NULL)
    {
        fprintf(stderr, "error: config already has a registered log file (%s)\n", config->currentLogFile);
        return -1;
    }

    /*
    // get the timestamp
    char *timeStamp;
    if (getTimeStamp(&timeStamp) != 0)
    {
        return -1;
    }

    // TODO: the logTag doesn't need to be hardcoded
    char *logTag = "antman-";
    char *logExt = ".log";
    char *logFile = malloc(1 + strlen(timeStamp) + strlen(logTag) + strlen(logExt));
    if (logFile == NULL)
    {
        fprintf(stderr, "error: could not allocate memory for log filename\n");
        free(timeStamp);
        return -1;
    }
    strcpy(logFile, logTag);
    strcat(logFile, timeStamp);
    strcat(logFile, logExt);
*/

    // set the log filename
    int retVal = setConfigField(config, 0, AM_DEFAULT_LOG);
    //free(timeStamp);
    //free(logFile);
    if (retVal)
        return retVal;

    // write the updates
    return writeConfig(config, config->filename);
}

/*****************************************************************************
 * setupDB will set up a reference database using the config.
 * 
 * available dbType:
 *      0 - bigsi
 * 
 * returns:
 *      0 on success, -1 on error
 */
int setupDB(config_t *config, int dbType)
{
    // TODO: only one type of database supported for now
    if (dbType != 0)
    {
        fprintf(stderr, "error: unrecognised database type for references\n");
        return -1;
    }
    if (bfCalc(config->maxElements, config->fpRate, &config->numBits, &config->numHashes))
    {
        fprintf(stderr, "error: could not calculate the required number of bits and hashes\n");
        return -1;
    }
    if ((config->bigsi = bigsInit(config->numBits, config->numHashes, config->dbDir)) == NULL)
    {
        fprintf(stderr, "error: couldn't link config with a bigsi\n");
    }
    return 0;
}

/*****************************************************************************
 * loadDB will load a reference database using the config.
 * 
 * available dbType:
 *      0 - bigsi
 * 
 * returns:
 *      0 on success, -1 on error
 */
int loadDB(config_t *config, int dbType)
{
    // TODO: only one type of database supported for now
    if (dbType != 0)
    {
        fprintf(stderr, "error: unrecognised database type for references\n");
        return -1;
    }
    if (checkConfig(config, true))
        return -1;

    if ((config->bigsi = bigsLoad(config->dbDir)) == NULL)
        return -1;
    return 0;
}