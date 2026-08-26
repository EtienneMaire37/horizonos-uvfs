#pragma once

#include "../inode.h"
#include <stdlib.h>

static inline void free_fs_specific(inode_t* inode)
{
    free(inode->fs_specific);
}
