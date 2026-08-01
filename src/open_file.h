#pragma once

#include "ref.h"
#include "vnode.h"

typedef struct
{
    int flags;
    vnode_ref_t vnode;
    struct stat st;
    off_t offset;
    atomic_flag lock;
    struct ref ref;
} open_file_descriptor_t;

typedef struct
{
    open_file_descriptor_t* _Atomic ptr;
} open_file_descriptor_ref_t;

#define OPEN_FD_REF_INIT ((struct ref){open_file_descriptor_free, 0})

void open_file_descriptor_free(const struct ref* ref);

open_file_descriptor_ref_t vfs_allocate_new_open_file_descriptor(int flags, vnode_ref_t vnode, struct stat* st);
void vfs_free_open_file_descriptor(int ofd);
