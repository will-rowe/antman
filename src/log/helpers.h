#include <stdio.h>
#include <time.h>

// getTimestamp returns the current time, formatted into a stamp
char* getTimestamp() {
    time_t timer;
    static char buffer[26];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y-%m-%d-%H-%M-%S", tm_info);
    puts(buffer);
    return buffer;
}
