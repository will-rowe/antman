/*****************************************************************************
 * Package helpers contains some helper functions.
 */
#ifndef HELPERS_H
#define HELPERS_H

#include <stdbool.h>

#include "3rd-party/slog.h"

/*****************************************************************************
 * function prototypes
 */
int checkDirectory(const char *dirName, bool create);
int checkFilePath(const char *filePath);
int getTimeStamp(char **timeStampPtr);
void slog_get_date(SlogDate *pDate);
char *getExtension(const char *filename);

#endif