#include "errors.h"

// Error messages
const char *const ERROR_STRINGS[] =
    {
        "SUCCESS",                    // 0
        "UNUSED ERROR CODE",          // 1
        "memory allocation failed",   // 2
        "empty argument to function", // 3
        "can't access file",          // 4
        "UNUSED ERROR CODE",          // 5
        "UNUSED ERROR CODE",          // 6
        "UNUSED ERROR CODE",          // 7
        "UNUSED ERROR CODE",          // 8
        "UNUSED ERROR CODE",          // 9
        "UNUSED ERROR CODE",          // 10
        "BIGSI has not been indexed", // 11
        "BIGSI num hash does not match query vector",
        "BIGSI num colours does not match result bit vector",
        "BIGSI row missing for query hash value",
        "BIGSI bitwise OR failed for query",
        "BIGSI bitwise AND failed for query",
};

// printError
const char *printError(ERROR errorCode)
{
    const char *error = NULL;
    if (errorCode >= ERROR_COUNT)
    {
        errorCode = ERROR_UNUSED;
    }
    error = ERROR_STRINGS[errorCode];
    return error;
}