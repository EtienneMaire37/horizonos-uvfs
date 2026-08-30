#pragma once

#include "mountpoint_ref.h"
#include "vnode_ref.h"
#include "ref.h"
#include "inode.h"
#include <sys/types.h>

struct mountpoint
{
    struct ref ref;
    
    vnode_ref_t root;
    dev_t dev;
    vnode_ref_t dev_node;
    blksize_t blksize;

    ino_t (*_Atomic generate_ino)();
    void* (*_Atomic create_inode)(const char*, vnode_ref_t, const struct stat*);
    void (*_Atomic free_inode)(void*);

    int (*_Atomic explore)(vnode_ref_t); 
    ssize_t (*_Atomic read)(vnode_ref_t, void*, size_t, off_t); 
    ssize_t (*_Atomic write)(vnode_ref_t, void*, size_t, off_t); 
    void (*_Atomic flush)(vnode_ref_t);
};

#define MOUNTPOINT_REF_INIT ((struct ref){ .count = 1, .free = ___mountpoint_free })

typedef enum
{
    FSTYPE_VIRTUAL,
    FSTYPE_INITRD,
    FSTYPE_UNKNOWN
} fstype_t;

mountpoint_ref_t mountpoint_copy_ref(mountpoint_ref_t ref);
void mountpoint_delete_ref(mountpoint_ref_t* ref);
void ___mountpoint_free(const struct ref* ref);
