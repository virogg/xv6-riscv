#include "logger.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define LOG_BUF_SIZE 512
#define TIMESTAMP_SIZE 20

FILE* log_stream = NULL;

void log_init(const char *log_file) {
    if (log_file) {
        log_stream = fopen(log_file, "a");
        if (!log_stream) {
            perror("Failed to open log file");
            exit(EXIT_FAILURE);
        }
    } else {
        log_stream = stdout;
    }
}

void log_close(void) {
    if (log_stream && log_stream != stdout) {
        fclose(log_stream);
    }
}

void log_message(LogLevel level, const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    const char *level_str = "INFO";
    switch (level) {
        case LOG_ERROR: level_str = "ERROR"; break;
        case LOG_FATAL: level_str = "FATAL"; break;
        default: break;
    }

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[TIMESTAMP_SIZE];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    char message[LOG_BUF_SIZE];
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    if (log_stream) {
        fprintf(log_stream, "[%s] %-5s %s:%d: %s\n", timestamp, level_str, file, line, message);
        fflush(log_stream);
    }

    if (level == LOG_FATAL) {
        log_close();
        exit(EXIT_FAILURE);
    }
}