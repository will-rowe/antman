
#include <stdlib.h>
#include <unistd.h>

#include "errors.h"
#include "watcher.h"

/*****************************************************************************
 * wargsInit will initiate the watcher arguments, which are used by the
 * libfswatch file watcher to process sequence files.
 * 
 * arguments:
 *      tp          -   a pointer to an initiated threadpool
 *      config      -   an initated and populated in-memory config
 * 
 * returns:
 *      an initiated watcherArgs_t struct
 * 
 * note:
 *      the user must free the returned watcherArgs_t
 *      no checking is done of the tp or config arguments
 */
watcherArgs_t *wargsInit(tpool_t *tp, config_t *config)
{
    watcherArgs_t *wargs;
    if ((wargs = malloc(sizeof(watcherArgs_t))) != NULL)
    {
        wargs->threadPool = tp;
        wargs->config = config;
    }
    return wargs;
}

/*****************************************************************************
 * wargsDestroy will free the watcher argument struct, as well as the filepath
 * if there is one.
 * 
 * arguments:
 *      wargs          -   a pointer to an initiated watcherArgs_t
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *      it will not free any config or threadpool attached to the struct
 */
int wargsDestroy(watcherArgs_t *wargs)
{
    if (wargs)
    {
        tpool_destroy(wargs->threadPool);
        configDestroy(wargs->config);
        free(wargs);
        return 0;
    }
    return ERROR_NULL_ARGUMENT;
}

/*****************************************************************************
 * wjobCreate will create a new job in response to a file being found. It will
 * check a filepath, initialise a watcherJob_t and add the file.
 * 
 * arguments:
 *      wargs          -   a pointer to an initiated watcherArgs_t
 *      fp             -   a file path to check and attach
 *      resultPtr      -   a pointer to a watchJob_t pointer, for the result
 * 
 * returns:
 *      0 on success, or an error code
 * 
 * note:
 *      the user must check and free the returned watcherJob_t
 *      no checking is done of the tp or config arguments
 */
int wjobCreate(watcherArgs_t *wargs, char *fp, watcherJob_t **resultPtr)
{
    if (!wargs || !fp)
        return ERROR_NULL_ARGUMENT;

    // check the file
    if (access(fp, R_OK | W_OK) == -1)
    {
        return ERROR_ACCESS;
    }

    // set up the job
    if (((*resultPtr) = malloc(sizeof(watcherJob_t))) == NULL)
        return ERROR_MALLOC;
    (*resultPtr)->wargs = wargs;
    (*resultPtr)->filepath = strdup(fp);
    return 0;
}

/*****************************************************************************
 * wjobDestroy will free the watcher job struct.
 * 
 * arguments:
 *      wjob          -   a pointer to an initiated watcherJob_t
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *      it will not free any config or threadpool attached to the struct
 */
int wjobDestroy(watcherJob_t *wjob)
{
    if (wjob)
    {
        free(wjob->filepath);
        free(wjob);
        return 0;
    }
    return ERROR_NULL_ARGUMENT;
}
