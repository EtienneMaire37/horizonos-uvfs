#pragma once

#include <stdint.h>

static inline uint64_t hash_string(const char* str)
{
    const uint64_t p = 97;
    const uint64_t m = (uint64_t)-59;
    uint64_t sum = 0, mul = 1;
    while (*str)
    {
        sum += *str * mul;
        mul *= p;
        str++;
    }
    return sum % m;
}
