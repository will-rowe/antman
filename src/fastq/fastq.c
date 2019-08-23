#include "../log/slog.h"

//
void justPrintMOFO(void *arg) {
    char* filepath = (char*)arg;
    slog(0, SLOG_INFO, "\t- a worker received the file ---> %s", filepath);
}
