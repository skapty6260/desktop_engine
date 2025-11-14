#include "logger.h"

void log_message(log_level level, const char* text, ...) {
    if (!text) {
        fprintf(stderr, "Invalid log message\n");
        return;
    }

    time_t now = time(NULL);
    printf("[%s] (%s) %s\n", ctime(&now), level, text);
}