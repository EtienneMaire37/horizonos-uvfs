#pragma once

#include <bits/types/siginfo_t.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <stdbool.h>

#include "ref.h"
#include "flags.h"

typedef struct vnode vnode_t;
struct vnode
{
    char* name;
    struct stat st;
    vnode_t *_Atomic children, *_Atomic next, *_Atomic prev, *_Atomic parent;
    atomic_flag lock;
    vnode_flags_t flags;
    struct ref ref;
};

#define VNODE_REF_INIT ((struct ref){vnode_free, 0})

extern vnode_t* _Atomic vfs_root_node;
extern _Atomic  size_t vfs_total_nodes;

void vfs_create_root_node();

void vnode_free(const struct ref* ref);
vnode_t* vfs_create_new_vnode(const char* name, const struct stat* st);
void vfs_add_new_child_node(vnode_t* node, const char* name, const struct stat* st);
void vfs_unload_children(vnode_t* node);
void vfs_log_structure(vnode_t* node);
void vfs_get_absolute_path_to_node(vnode_t* node, char* buf, size_t bufsiz);
bool vfs_verify_tree_integrity();

ino_t vfs_generate_ino();
