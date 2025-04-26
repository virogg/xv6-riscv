#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>

typedef enum {
    LOG_INFO,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

extern FILE* log_stream;

void log_init(const char *log_file);
void log_reopen(void);
void log_close(void);
void log_message(LogLevel level, const char *file, int line, const char *fmt, ...);

#define LOG_INFO(...) log_message(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)