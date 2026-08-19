#include "vnode.h"
#include "explore.h"
#include "flags.h"
#include "mountpoint_ref.h"
#include "ref.h"
#include "spinlock.h"
#include "log.h"
#include "util/string.h"
#include "mountpoint.h"
#include "fs/virtual.h"
#include "rdev.h"
#include "vnode_ref.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

vnode_ref_struct_t vfs_root_node = (vnode_ref_struct_t){ NULL };
_Atomic size_t vfs_total_nodes = 0;

void vfs_create_root_node()
{
    struct stat st;
    st.st_atim = (struct timespec){ 0, 0 };
    st.st_ctim = (struct timespec){ 0, 0 };
    st.st_mtim = (struct timespec){ 0, 0 };
    st.st_blksize = 4096;
    st.st_blocks = 0;
    st.st_dev = 0;
    st.st_rdev = 0;

    st.st_gid = 0;
    st.st_uid = 0;

    st.st_ino = virtfs_generate_ino();
    st.st_mode = 0755 | S_IFDIR;

    st.st_nlink = 1;
    st.st_size = 0;

    vfs_root_node = vnode_struct_dereference(vfs_create_new_vnode("/", &st));
}

vnode_ref_t vfs_create_new_vnode(const char* name, const struct stat* st)
{
    assert(name && st);
    vnode_t* newn = calloc(1, sizeof(vnode_t));
    if (!newn) return (vnode_ref_t){ NULL };
    newn->name = strdup(name);
    if (!newn->name)
    {
        free(newn);
        return (vnode_ref_t){ NULL };
    }
    vfs_total_nodes++;
    newn->st = *st;
    newn->lock = (atomic_flag)ATOMIC_FLAG_INIT;
    newn->flags = VNODE_INIT;
    newn->ref = VNODE_REF_INIT;
    return (vnode_ref_t){ newn };
}

void vnode_delete_ref(vnode_ref_t* ref)
{
    if (!ref || !ref->ptr) return;
    LOG(TRACE, "Deleting reference to node \"%s\"", ref->ptr->name);
    struct ref* ref_ref = ref->ptr ? &ref->ptr->ref : NULL;
    ref->ptr = NULL;
    if (ref_ref)
        ref_dec(ref_ref);
}

vnode_ref_t ___vnode_dereference(vnode_ref_t node, size_t field_offset)
{
    assert(node.ptr);
    uint32_t flags = acquire_spinlock_noint(&node.ptr->lock);
    vnode_t* field_value = *(vnode_t**)((uintptr_t)node.ptr + field_offset);
    if (field_value)
        ref_inc(&field_value->ref);
    release_spinlock_noint(&node.ptr->lock, flags);
    return (vnode_ref_t){ field_value };
}

void ___vnode_move_reference(vnode_ref_t* ref, size_t field_offset)
{
    assert(ref && ref->ptr);
    vnode_ref_t new_ref = ___vnode_dereference(*ref, field_offset);
    vnode_delete_ref(ref);
    *ref = new_ref;
}

void vfs_add_new_child_node(vnode_ref_t node, const char* name, struct stat st)
{
    assert(node.ptr);

    if (!S_ISDIR(node.ptr->st.st_mode)) return;

    st.st_blksize = node.ptr->mountpoint.ptr->blksize;
    st.st_dev = node.ptr->mountpoint.ptr->dev;

    vnode_ref_t child = vfs_create_new_vnode(name, &st);

    if (!child.ptr)
    {
        LOG(WARN, "vfs_add_new_child_node: Couldn't allocate child");
        return;
    }

    child.ptr->mountpoint = mountpoint_struct_dereference(mountpoint_copy_ref(mountpoint_ref_dereference(node.ptr->mountpoint)));

    uint32_t node_flags = node.ptr->flags;
    if (!(node_flags & VNODE_EXPLORED) && !(node_flags & VNODE_EXPLORING))
        vfs_explore(node);

    vnode_ref_t ref = vnode_dereference(node, children);
    uint32_t flags = acquire_spinlock_noint(&node.ptr->lock);

    child.ptr->next = ref.ptr;
    child.ptr->parent = node.ptr;

    if (ref.ptr)
    {
        uint32_t flags = acquire_spinlock_noint(&ref.ptr->lock);
        child.ptr->next->prev = child.ptr;
        node.ptr->children.ptr = child.ptr;
        release_spinlock_noint(&ref.ptr->lock, flags);
    }
    else
        node.ptr->children.ptr = child.ptr;

    release_spinlock_noint(&node.ptr->lock, flags);

    vnode_delete_ref(&ref);
}

void vfs_unload_children(vnode_ref_t node)
{
    assert(node.ptr);

    vnode_ref_t child = vnode_dereference(node, children);
    node.ptr->children.ptr = NULL;
    while (child.ptr)
    {
        uint32_t flags = acquire_spinlock_noint(&child.ptr->lock);
        if (child.ptr->next)
        {
            uint32_t flags = acquire_spinlock_noint(&child.ptr->next->lock);
            child.ptr->next->prev = NULL;
            release_spinlock_noint(&child.ptr->next->lock, flags);
        }
        release_spinlock_noint(&child.ptr->lock, flags);
        ref_dec(&child.ptr->ref);
        vnode_move_reference(&child, next);
    }
}

void vfs_unparent_children(vnode_ref_t node)
{
    assert(node.ptr);

    vnode_ref_t child = vnode_dereference(node, children);
    while (child.ptr)
    {
        child.ptr->parent = NULL;
        vnode_move_reference(&child, next);
    }
}

void ___vnode_free(const struct ref* _ref)
{
    vnode_t* node = container_of(_ref, vnode_t, ref);
    LOG(DEBUG, "Freeing vnode \"%s\"", node->name);
    if (node == vfs_root_node.ptr)
    {
        LOG(FATAL, "vnode_free: Tried to free root node");
        abort();
    }
    vnode_ref_t ref = { node };
    vfs_unparent_children(ref);
    vfs_unload_children(ref);
    free(node->name);
    free(node);
    vfs_total_nodes--;
}

void vfs_log_structure_helper(vnode_ref_t node, int depth)
{
    assert(node.ptr);

    LOG(DEBUG, "%*s- \"%s\" (inode %ld) [refcount %d]%s", depth, "", node.ptr->name, (long)node.ptr->st.st_ino, node.ptr->ref.count - 1, ((node.ptr->flags & VNODE_EXPLORED) || (!S_ISDIR(node.ptr->st.st_mode))) ? "" : " <NOT EXPLORED>");
    vnode_ref_t child = vnode_dereference(node, children);
    while (child.ptr)
    {
        vfs_log_structure_helper(child, depth + 4);
        vnode_move_reference(&child, next);
    }
}

void vfs_log_structure(vnode_ref_t node)
{
    vfs_log_structure_helper(node, 0);
}

size_t vfs_get_absolute_path_to_node_helper(vnode_ref_t ref, char* buf, size_t bufsiz)
{
    vnode_t* node = ref.ptr;
    assert(node);
    LOG(TRACE, "vfs_get_absolute_path_to_node_helper: %s", node->name);
    size_t offset = 0;
    vnode_ref_t parent_ref = vnode_dereference(ref, parent);
    if (parent_ref.ptr)
    {
        offset = vfs_get_absolute_path_to_node_helper(parent_ref, buf, bufsiz);
        vnode_delete_ref(&parent_ref);
        if (offset < bufsiz)
        {
            int maxwrite = bufsiz - offset - 1; // -1 to take the / into account
            int len = strlen(node->name);
            offset += snprintf(&buf[offset], bufsiz, "/%*s", len > maxwrite ? maxwrite : len, node->name);
        }
    }
    return offset;
}

size_t vfs_get_absolute_path_to_node(vnode_ref_t ref, char* buf, size_t bufsiz)
{
    vnode_t* node = ref.ptr;
    assert(bufsiz > 2);
    assert(buf);
    assert(node);
    size_t ret;
    if (node->parent)
        buf[(ret = vfs_get_absolute_path_to_node_helper(ref, buf, bufsiz - 1) + 1)] = 0;
    else
        strcpy(buf, (ret = 2, "/"));
    return ret;
}

size_t vfs_count_nodes(vnode_ref_t ref)
{
    vnode_t* node = ref.ptr;
    assert(node);
    size_t total = 0;
    vnode_ref_t child = vnode_dereference(ref, children);
    while (child.ptr)
    {
        total++;
        total += vfs_count_nodes(child);
        vnode_move_reference(&child, next);
    }
    return total;
}

bool vfs_verify_tree_integrity()
{
    size_t nodes = vfs_count_nodes(vnode_ref_dereference(vfs_root_node)) + 1;
    bool ret = nodes == vfs_total_nodes;
    #ifndef NDEBUG
    if (!ret)
    {
        LOG(DEBUG, "Total refcounted nodes: %zu", vfs_total_nodes);
        LOG(DEBUG, "Total nodes in tree: %zu", nodes);
        abort();
    }
    #endif
    return ret;
}

vnode_ref_t vnode_copy_ref(vnode_ref_t ref)
{
    if (!ref.ptr) return ref;
    ref_inc(&ref.ptr->ref);
    return ref;
}

vnode_ref_t vfs_get_vnode_from_path(int* _errno, uid_t uid, gid_t gid, const char* path, vnode_ref_t root, vnode_ref_t cwd, bool follow_symlinks)
{
    assert(_errno);
    assert(path);
    if (!root.ptr) root = vnode_ref_dereference(vfs_root_node);
    LOG(TRACE, "Searching for vnode with path \"%s\"", path);
    *_errno = 0;
    vnode_ref_t ecwd = (cwd.ptr && !S_ISDIR(cwd.ptr->st.st_mode)) ? vnode_dereference(cwd, parent) : vnode_copy_ref(cwd);
    bool absolute_path = *path == '/';
    while (*path == '/') path++;
    if (!*path)
    {
        vnode_delete_ref(&ecwd);
        return absolute_path ? vnode_copy_ref(root) : vnode_copy_ref(ecwd);
    }
    vnode_ref_t current = (ecwd.ptr && !absolute_path) ? vnode_copy_ref(ecwd) : vnode_copy_ref(root);
    if ((*_errno = vfs_explore(current)))
    {
        vnode_delete_ref(&ecwd);
        vnode_delete_ref(&current);
        return (vnode_ref_t){ NULL };
    }
    bool x = (uid == current.ptr->st.st_uid) ? (current.ptr->st.st_mode & S_IXUSR) :
            ((gid == current.ptr->st.st_gid) ? (current.ptr->st.st_mode & S_IXGRP) :
                                             (current.ptr->st.st_mode & S_IXOTH));
    if (uid != 0 && !x)
    {
        vnode_delete_ref(&ecwd);
        vnode_delete_ref(&current);
        *_errno = EACCES;
        return (vnode_ref_t){ NULL };
    }

    vnode_ref_t child = vnode_dereference(current, children);
    size_t len = strlen_slash(path);
    int fake_entries = 2;
    while (fake_entries || child.ptr)
    {
        if (fake_entries)
        {
            if (fake_entries == 2)
            {
                if (strcmp_slash(".", path) == 0)
                {
                    path += len;
                    while (*path == '/')
                        path++;
                    if (!*path)
                    {
                        vnode_delete_ref(&ecwd);
                        vnode_delete_ref(&current);
                        return current;
                    }
                    len = strlen_slash(path);
                    fake_entries = 2;
                    continue;
                }
            }
            else // fake_entries == 1
            {
                if (strcmp_slash("..", path) == 0)
                {
                    path += len;
                    while (*path == '/')
                        path++;
                    if (current.ptr != root.ptr)
                    {
                        vnode_move_reference(&current, parent);
                        bool x = (uid == current.ptr->st.st_uid) ? (current.ptr->st.st_mode & S_IXUSR) :
                                ((gid == current.ptr->st.st_gid) ? (current.ptr->st.st_mode & S_IXGRP) :
                                                                 (current.ptr->st.st_mode & S_IXOTH));
                        if (uid != 0 && !x)
                        {
                            vnode_delete_ref(&ecwd);
                            vnode_delete_ref(&current);
                            *_errno = EACCES;
                            return (vnode_ref_t){ NULL };
                        }
                        vnode_delete_ref(&child);
                        child = vnode_dereference(current, children);
                    }
                    if (!*path)
                    {
                        vnode_delete_ref(&ecwd);
                        return current;
                    }
                    len = strlen_slash(path);
                    fake_entries = 2;
                    continue;
                }
            }
            fake_entries--;
            continue;
        }
        if (strcmp_slash(child.ptr->name, path) == 0)
        {
            path += len;
            while (*path == '/')
                path++;
            vnode_delete_ref(&current);
            if ((*path && !S_ISDIR(child.ptr->st.st_mode)) || (!*path && S_ISLNK(child.ptr->st.st_mode)))
            {
                if (*path && !S_ISLNK(child.ptr->st.st_mode))
                {
                    vnode_delete_ref(&ecwd);
                    vnode_delete_ref(&current);
                    *_errno = ENOTDIR;
                    return (vnode_ref_t){ NULL };
                }
                vnode_ref_t _child = child;
                char _path[PATH_MAX];
                ssize_t ret;
                if ((ret = vfs_read(child, _path, sizeof(_path), 0)) < 0)
                {
                    vnode_delete_ref(&ecwd);
                    vnode_delete_ref(&current);
                    *_errno = -ret;
                    return (vnode_ref_t){ NULL };
                }
                child = vfs_get_vnode_from_path(_errno, uid, gid, _path, root, _child, follow_symlinks);
                vnode_delete_ref(&_child);
            }
            bool x = (uid == child.ptr->st.st_uid) ? (child.ptr->st.st_mode & S_IXUSR) :
                    ((gid == child.ptr->st.st_gid) ? (child.ptr->st.st_mode & S_IXGRP) :
                                                     (child.ptr->st.st_mode & S_IXOTH));
            if (!*path)
            {
                vnode_delete_ref(&ecwd);
                vnode_delete_ref(&current);
                return child;
            }
            if (uid != 0 && !x)
            {
                vnode_delete_ref(&ecwd);
                vnode_delete_ref(&current);
                *_errno = EACCES;
                return (vnode_ref_t){ NULL };
            }
            current = child;
            if ((*_errno = vfs_explore(current)))
            {
                vnode_delete_ref(&ecwd);
                vnode_delete_ref(&current);
                return (vnode_ref_t){ NULL };
            }
            child = vnode_dereference(current, children);
            len = strlen_slash(path);
            fake_entries = 2;
            continue;
        }
        vnode_move_reference(&child, next);
    }
    vnode_delete_ref(&current);
    vnode_delete_ref(&ecwd);
    *_errno = ENOENT;
    return (vnode_ref_t){ NULL };
}

ssize_t vfs_read(vnode_ref_t ref, void* buf, size_t bytes, off_t offset)
{
    vnode_t* node = ref.ptr;
    assert(node);
    assert(ref.ptr->read);
    return ref.ptr->read(ref, buf, bytes, offset);
}

ssize_t vfs_write(vnode_ref_t ref, void* buf, size_t bytes, off_t offset)
{
    vnode_t* node = ref.ptr;
    assert(node);
    assert(ref.ptr->write);
    return ref.ptr->write(ref, buf, bytes, offset);
}

int vfs_mount(vnode_ref_t ref, vnode_ref_t dev, const char* fstype)
{
    fstype_t fstype_en;
    if (strcmp(fstype, "virt") == 0)
        fstype_en = FSTYPE_VIRTUAL;
    else
        return EINVAL;
    
    if (!ref.ptr || (!dev.ptr && fstype_en != FSTYPE_VIRTUAL)) return ENOENT;
    mountpoint_t* mountpoint = malloc(sizeof(mountpoint_t));
    if (!mountpoint)
        return ENOMEM;
    mountpoint->ref = MOUNTPOINT_REF_INIT;
    mountpoint->dev = dev.ptr ? dev.ptr->st.st_rdev : vfs_generate_rdev();
    mountpoint->root = vnode_copy_ref(ref);
    switch (fstype_en)
    {
    case FSTYPE_VIRTUAL:
        mountpoint->generate_ino = virtfs_generate_ino;
        break;

    default:
        ;
    }
    uint32_t flags = acquire_spinlock_noint(&ref.ptr->lock);
    mountpoint_ref_t old_ref = { ref.ptr->mountpoint.ptr };
    ref.ptr->mountpoint.ptr = mountpoint;
    mountpoint_delete_ref(&old_ref);
    release_spinlock_noint(&ref.ptr->lock, flags);
    return 0;
}

int vfs_mkdir(const char* name, vnode_ref_t parent, mode_t access, uid_t uid, gid_t gid)
{
    mountpoint_t* mountpoint = parent.ptr->mountpoint.ptr;
    if (!mountpoint) return EPERM;
    struct stat st;
    st.st_mode = access | S_IFDIR;
    st.st_ino = mountpoint->generate_ino();
    st.st_blocks = 0;
    st.st_rdev = 0;
    st.st_uid = uid;
    st.st_gid = gid;
    st.st_nlink = 1;
    st.st_size = 0;
    st.st_atim = (struct timespec){ 0, 0 };
    st.st_ctim = (struct timespec){ 0, 0 };
    st.st_mtim = (struct timespec){ 0, 0 };
    vfs_add_new_child_node(parent, name, st);
    return 0;
}
