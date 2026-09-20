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

    ino_t (*generate_ino)();
    // void* (*create_inode)(const char*, vnode_ref_t, const struct stat*);
    // void (*free_inode)(void*);

    int (*explore)(vnode_ref_t); 
    ssize_t (*read)(vnode_ref_t, void*, size_t, off_t); 
    ssize_t (*write)(vnode_ref_t, void*, size_t, off_t);
    int (*create)(const char*, vnode_ref_t, struct stat*, void**, void (**)(void*));
    // TODO: Add more operations
    void (*flush)(vnode_ref_t);

    void* data;
    void (*free_data)(void*);
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
