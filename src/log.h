#pragma once

#include <stdio.h>

#ifdef LOG_LEVEL 

static const char* log_level[] =
{
    "[TRACE]",
    "[DEBUG]",
    "[INFO]",
    "[WARN]",
    "[ERROR]",
    "[FATAL]"
};

#define TRACE       0
#define DEBUG       1
#define INFO        2
#define WARN        3
#define ERROR       4
#define FATAL       5

#define LOG(level, ...) do { if (level >= LOG_LEVEL) { fprintf(stderr, "%s\t", log_level[level % (sizeof(log_level) / sizeof(log_level[0]))]); fprintf(stderr, __VA_ARGS__); putchar('\n'); } } while (0)
#else
#define LOG(level, ...)
#endif
