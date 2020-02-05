#ifndef ERRORS_H
#define ERRORS_H

#include <stdlib.h>

// Error codes
typedef enum
{
    SUCCESS = 0,
    ERROR_UNUSED,
    ERROR_MALLOC,
    ERROR_NULL_ARGUMENT,
    ERROR_ACCESS,

    // include an error count to keep track of the number of errors we can handle
    ERROR_COUNT,
} ERROR;

// Print error message
const char *printError(ERROR errorCode);

#endif