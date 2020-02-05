#include "errors.h"

// Error messages
const char *const ERROR_STRINGS[] =
    {
        "SUCCESS",
        "UNUSED ERROR CODE",
        "memory allocation failed",
        "missing argument to function",
        "can't access file",
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