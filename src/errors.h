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
    ERROR_UNUSED_5,
    ERROR_UNUSED_6,
    ERROR_UNUSED_7,
    ERROR_UNUSED_8,
    ERROR_UNUSED_9,
    ERROR_UNUSED_10,
    ERROR_BIGSI_UNINDEXED,
    ERROR_BIGSI_HASH_MISMATCH,
    ERROR_BIGSI_RESULT_MISMATCH,
    ERROR_BIGSI_MISSING_ROW,
    ERROR_BIGSI_OR_FAIL,
    ERROR_BIGSI_AND_FAIL,

    // include an error count to keep track of the number of errors we can handle
    ERROR_COUNT,
} ERROR;

// Print error message
const char *printError(ERROR errorCode);

#endif