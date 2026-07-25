#pragma once

#include <bits/types/siginfo_t.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <sys/types.h>

#include "flags.h"

typedef struct vnode vnode_t;
struct vnode
{
    char* name;
    struct stat st;
    vnode_t *_Atomic children, *_Atomic next, *_Atomic prev;
    atomic_flag lock;
    vnode_flags_t flags;
    ssize_t reference_count;
};

extern vnode_t* vfs_root_node;

vnode_t* vfs_create_new_vnode(const char* name, const struct stat* st);
void __vfs_unload_children(vnode_t* node);
void vfs_node_destroy(vnode_t* node);
void vfs_log_structure(vnode_t* node);
