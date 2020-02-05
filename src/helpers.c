#include <dirent.h>
#include <errno.h>
#include <libgen.h> /* basename */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "helpers.h"

/*****************************************************************************
 * checkDirectory is a helper function to check a user-supplied directory and
 * optionaly create it if it does not exist.
 */
int checkDirectory(const char *dirName, bool create)
{
    DIR *dir = opendir(dirName);
    if (dir)
    {
        closedir(dir);
        return 0;
    }
    else if (ENOENT == errno)
    {
        if (!create)
        {
            fprintf(stderr, "specified directory does not exist: %s\n", dirName);
            return -1;
        }
        if (mkdir(dirName, 0755))
        {
            fprintf(stderr, "could not create directory: %s\n", dirName);
            return -1;
        }
        return 0;
    }
    else
    {
        fprintf(stderr, "can't access the specified directory: %s\n", dirName);
        return -1;
    }
}

/*****************************************************************************
 * checkFilePath is a helper function to check a filepath and create it if it 
 * does not exist.
 */
int checkFilePath(const char *filePath)
{

    // check if it already exists
    if (access(filePath, R_OK | W_OK) == -1)
    {

        // get the base name and dir name
        char *dirc, *dname;
        dirc = strdup(filePath);
        dname = dirname(dirc);

        // create directories if needed
        if (dname != NULL)
        {
            char *str, *s;
            struct stat statBuf;
            s = dname;
            while ((str = strtok(s, "/")) != NULL)
            {
                if (str != s)
                {
                    str[-1] = '/';
                }
                if (stat(dname, &statBuf) == -1)
                    checkDirectory(dname, true);
                s = NULL;
            }
        }
        free(dirc);

        // create the file
        FILE *fPtr = fopen(filePath, "w");
        if (fPtr == NULL)
        {
            fprintf(stderr, "couldn't create file: %s\n", filePath);
            return -1;
        }
        fclose(fPtr);
    }

    return 0;
}

/*****************************************************************************
 * getTimeStamp will create a timestamp and give it to the provided pointer.
 * 
 * arguments:
 *      timeStampPtr              - a pointer to an uninitialised char* that will be used for the result
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *      it is the caller's responsibility to check and free the timestamp
 */
int getTimeStamp(char **timeStampPtr)
{
    // check we can send the result
    if (!timeStampPtr)
    {
        fprintf(stderr, "no pointer provided for returning timestamp\n");
        return -1;
    }

    // use slog to get the time
    SlogDate date;
    slog_get_date(&date);

    // set up the timestamp
    int timeStampLength = 18;
    if ((*timeStampPtr = malloc(timeStampLength * sizeof(char))) == NULL)
    {
        fprintf(stderr, "could not allocate memory for timestamp\n");
        return -1;
    }

    if (sprintf(*timeStampPtr, "%d-%d-%d-%d%d", date.year, date.mon, date.day, date.hour, date.min) > timeStampLength)
    {
        fprintf(stderr, "failed to format time stamp\n");
        return -1;
    }
    return 0;
}

/*****************************************************************************
 * getExtension takes a filename and returns the extension (if there is one).
 */
char *getExtension(const char *filename)
{
    // strrchr finds the final occurance of a char
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}