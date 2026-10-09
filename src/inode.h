#pragma once

#include "mountpoint_ref.h"
#include "ref.h"
#include "inode_ref.h"
#include "vnode_ref.h"
#include "spinlock.h"
#include <sys/stat.h>

struct inode
{
    struct ref ref;
    spinlock_noint_t lock;

    mountpoint_ref_t mountpoint;

    void* fs_specific;
    void (*free_fs_specific_data)(void*);
    struct stat st;
};

#define INODE_REF_INIT ((struct ref){___inode_free, 1})

void ___inode_free(const struct ref* _ref);
inode_ref_t vfs_create_new_inode(const struct stat* st, void* fs_specific, void (*free_fs_specific_data)(void*), mountpoint_ref_t mp);
void inode_delete_ref(inode_ref_t* ref);
inode_ref_t inode_copy_ref(inode_ref_t ref);
