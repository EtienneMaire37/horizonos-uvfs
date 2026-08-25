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
#include "inode_ref.h"

struct vnode
{
    struct ref ref;

    char* _Atomic name;

    inode_ref_t inode;

    vnode_ref_t children; // Owns a reference to each child
    vnode_t *_Atomic next, *_Atomic parent; // Non owning references to other nodes
    // ! No "prev" as it would cause problems to make the system MT-safe and is not really useful
    
    atomic_flag lock;
    vnode_flags_t flags;
    mountpoint_ref_t mountpoint;

    int (*_Atomic explore)(vnode_ref_t); 
    ssize_t (*_Atomic read)(vnode_ref_t, void*, size_t, off_t); 
    ssize_t (*_Atomic write)(vnode_ref_t, void*, size_t, off_t); 
};

#define vnode_dereference_vnode(vnode, field)       ___vnode_dereference_vnode((vnode), offsetof(vnode_t, field))
#define vnode_dereference_inode(vnode, field)       ___vnode_dereference_inode((vnode), offsetof(vnode_t, field))
#define vnode_dereference_mountpoint(vnode, field)       ___vnode_dereference_mountpoint((vnode), offsetof(vnode_t, field))
#define vnode_move_reference(vnode, field)       ___vnode_move_reference((vnode), offsetof(vnode_t, field))

#define VNODE_REF_INIT ((struct ref){___vnode_free, 1})

extern vnode_ref_t vfs_root_node;
extern _Atomic  size_t vfs_total_nodes;

void vfs_create_root_node();

vnode_ref_t ___vnode_dereference_vnode(vnode_ref_t node, size_t field_offset);
inode_ref_t ___vnode_dereference_inode(vnode_ref_t node, size_t field_offset);
mountpoint_ref_t ___vnode_dereference_mountpoint(vnode_ref_t node, size_t field_offset);
void vnode_delete_ref(vnode_ref_t* ref);
void ___vnode_move_reference(vnode_ref_t* ref, size_t field_offset);

void ___vnode_free(const struct ref* ref);
vnode_ref_t vfs_create_new_vnode(const char* name, inode_ref_t inode);
int vfs_add_new_child_node(vnode_ref_t node, const char* name, const struct stat* st, void* fs_specific, void (*free_fs_specific_data)(inode_t*));
int vfs_add_new_child_node_ex(vnode_ref_t node, const char* name, struct stat st,
    ssize_t (*read)(vnode_ref_t, void*, size_t, off_t), ssize_t (*write)(vnode_ref_t, void*, size_t, off_t),
    void* fs_specific, void (*free_fs_specific_data)(inode_t*));
int vfs_add_new_special_child_node(vnode_ref_t node, const char* name, mode_t mode, uid_t uid, gid_t gid,
    ssize_t (*read)(vnode_ref_t, void*, size_t, off_t), ssize_t (*write)(vnode_ref_t, void*, size_t, off_t),
    void* fs_specific, void (*free_fs_specific_data)(inode_t*));
void vfs_unload_children(vnode_ref_t node);
void vfs_log_structure(vnode_ref_t node);
size_t vfs_get_absolute_path_to_node(vnode_ref_t node, char* buf, size_t bufsiz);
bool vfs_verify_tree_integrity();
vnode_ref_t vnode_copy_ref(vnode_ref_t ref);
vnode_ref_t vfs_get_vnode_from_path(int* _errno, uid_t uid, gid_t gid, const char* path, vnode_ref_t root, vnode_ref_t cwd, bool follow_symlinks);
void vfs_unmount(vnode_ref_t ref);

ssize_t vnode_read(vnode_ref_t ref, void* buf, size_t bytes, off_t offset);
ssize_t vnode_write(vnode_ref_t ref, void* buf, size_t bytes, off_t offset);

int vfs_mount(vnode_ref_t ref, vnode_ref_t dev, const char* fstype);
int vfs_mkdir(const char* name, vnode_ref_t parent, mode_t access, uid_t uid, gid_t gid);
