#pragma once

#include <bits/types/siginfo_t.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>

#include "ref.h"
#include "flags.h"
#include "vnode_ref.h"
#include "mountpoint_ref.h"

struct vnode
{
    char* _Atomic name;
    struct stat st;

    vnode_ref_t children; // Owns a reference to each child
    vnode_t *_Atomic next, *_Atomic prev, *_Atomic parent; // Non owning references to other nodes
    
    atomic_flag lock;
    vnode_flags_t flags;
    struct ref ref;
    void* fs_specific;
    mountpoint_ref_t mountpoint;

    int (*_Atomic explore)(vnode_ref_t); 
    ssize_t (*_Atomic read)(vnode_ref_t, void*, size_t, off_t); 
    ssize_t (*_Atomic write)(vnode_ref_t, void*, size_t, off_t); 
};

#define vnode_dereference(vnode, field)       ___vnode_dereference((vnode), offsetof(vnode_t, field))
#define vnode_move_reference(vnode, field)       ___vnode_move_reference((vnode), offsetof(vnode_t, field))

#define VNODE_REF_INIT ((struct ref){___vnode_free, 1})

extern vnode_ref_t vfs_root_node;
extern _Atomic  size_t vfs_total_nodes;

void vfs_create_root_node();

vnode_ref_t ___vnode_dereference(vnode_ref_t node, size_t field_offset);
void vnode_delete_ref(vnode_ref_t* ref);
void ___vnode_move_reference(vnode_ref_t* ref, size_t field_offset);

void ___vnode_free(const struct ref* ref);
vnode_ref_t vfs_create_new_vnode(const char* name, const struct stat* st);
void vfs_add_new_child_node(vnode_ref_t node, const char* name, const struct stat* st);
void vfs_unload_children(vnode_ref_t node);
void vfs_log_structure(vnode_ref_t node);
size_t vfs_get_absolute_path_to_node(vnode_ref_t node, char* buf, size_t bufsiz);
bool vfs_verify_tree_integrity();
vnode_ref_t vfs_copy_reference(vnode_ref_t ref);
vnode_ref_t vfs_get_vnode_from_path(int* _errno, uid_t uid, gid_t gid, const char* path, vnode_ref_t root, vnode_ref_t cwd, bool follow_symlinks);

ssize_t vfs_read(vnode_ref_t ref, void* buf, size_t bytes, off_t offset);
ssize_t vfs_write(vnode_ref_t ref, void* buf, size_t bytes, off_t offset);

int vfs_mount(vnode_ref_t ref, vnode_ref_t dev, const char* fstype);
int vfs_mkdir(const char* name, vnode_ref_t parent, mode_t access, uid_t uid, gid_t gid);
