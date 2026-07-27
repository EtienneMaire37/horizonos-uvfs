#pragma once

#include <stdio.h>

static const char* log_level[] =
{
    "[TRACE]",
    "[DEBUG]",
    "[INFO]",
    "[WARN]",
    "[ERROR]"
};

#define TRACE       0
#define DEBUG       1
#define INFO        2
#define WARN        3
#define ERROR       4

#define LOG(level, ...) do { fprintf(stderr, "%s\t", log_level[level % (sizeof(log_level) / sizeof(log_level[0]))]); fprintf(stderr, __VA_ARGS__); putchar('\n'); } while (0)
