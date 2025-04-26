#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define LOG_BUF_SIZE 512
#define TIMESTAMP_SIZE 20

FILE *log_stream = NULL;

static const char *level_str(LogLevel lvl) {
    switch (lvl) {
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default:        return "INFO";
    }
}

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
    setvbuf(log_stream, NULL, _IOLBF, 0);
}

void log_reopen(void) {
    if (log_stream && log_stream != stdout) {
        fclose(log_stream);
    }
    log_stream = fdopen(STDOUT_FILENO, "a");
    if (!log_stream) {
        perror("fdopen after daemonize failed");
        log_stream = stdout;
    }
    setvbuf(log_stream, NULL, _IOLBF, 0);
}

void log_close(void) {
    if (log_stream && log_stream != stdout) {
        fclose(log_stream);
    }
}

void log_message(LogLevel level, const char *file, int line, const char *fmt, ...) {
    if (!log_stream) return;

    char timestamp[TIMESTAMP_SIZE];
    time_t now = time(NULL);
    strftime(timestamp, sizeof timestamp, "%Y-%m-%d %H:%M:%S", localtime(&now));

    char message[LOG_BUF_SIZE];
    va_list ap; va_start(ap, fmt);
    vsnprintf(message, sizeof message, fmt, ap);
    va_end(ap);

    fprintf(log_stream, "[%s] %-5s %s:%d: %s\n", timestamp, level_str(level), file, line, message);
    fflush(log_stream);

    if (level == LOG_FATAL) {
        log_close();
        _exit(EXIT_FAILURE);
    }
}