#include "virtual.h"

ino_t virtfs_generate_ino()
{
    static ino_t num = 1;
    return num++;
}

