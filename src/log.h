#pragma once

#include <stdio.h>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); putchar('\n'); } while (0)
