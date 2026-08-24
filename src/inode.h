#pragma once

#include "ref.h"
#include "inode_ref.h"
#include "vnode_ref.h"
#include <stdatomic.h>
#include <sys/stat.h>

struct inode
{
    struct ref ref;
    atomic_flag lock;

    void* fs_specific;
    void (*free_fs_specific_data)(struct inode*);
    struct stat st;
};

#define INODE_REF_INIT ((struct ref){___inode_free, 1})

void ___inode_free(const struct ref* _ref);
inode_ref_t vfs_create_new_inode(const struct stat* st, void* fs_specific, void (*free_fs_specific_data)(inode_t*));
void inode_delete_ref(inode_ref_t* ref);
inode_ref_t inode_copy_ref(inode_ref_t ref);
