#pragma once

#include <stdio.h>

#define LOG_MACRO(level, required_level, ...) do { if (level >= required_level) { fprintf(stderr, "[%s]\t[%s:%d]\t%s\t", __func__, __FILE__, __LINE__, log_level[level % (sizeof(log_level) / sizeof(log_level[0]))]); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

static const char* log_level[] =
{
    "\033[1;38;5;240m[TRACE]\033[0m",
    "\033[1;38;5;25m[DEBUG]\033[0m",
    "\033[1;38;5;11m[INFO]\033[0m",
    "\033[1;38;5;124m[WARN]\033[0m",
    "\033[1;38;5;160m[ERROR]\033[0m",
    "\033[1;38;5;196m[FATAL]\033[0m"
};

#define TRACE       0
#define DEBUG       1
#define INFO        2
#define WARN        3
#define ERROR       4
#define FATAL       5

#ifdef LOG_LEVEL 
#define LOG(level, ...) LOG_MACRO(level, LOG_LEVEL, __VA_ARGS__)
#else
// Do this to have LSP support
#define LOG(level, ...) LOG_MACRO(level, -1, __VA_ARGS__)
#endif
