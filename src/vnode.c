#include "vnode.h"
#include "flags.h"
#include "spinlock.h"
#include "log.h"
#include "util/string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

vnode_ref_t vfs_root_node = (vnode_ref_t){ NULL };
_Atomic size_t vfs_total_nodes = 0;

void vfs_create_root_node()
{
    struct stat st;
    st.st_atim = (struct timespec){0, 0};
    st.st_ctim = (struct timespec){0, 0};
    st.st_mtim = (struct timespec){0, 0};
    st.st_blksize = 4096;
    st.st_blocks = 0;
    st.st_dev = 0;
    st.st_rdev = 0;

    st.st_gid = 0;
    st.st_uid = 0;

    st.st_ino = 1;
    st.st_mode = S_IFDIR | S_IRWXO | S_IRWXG;

    st.st_nlink = 1;
    st.st_size = 0;

    vnode_t* node = vfs_create_new_vnode("/", &st);
    ref_inc(&node->ref);
    vfs_root_node = (vnode_ref_t){ node };
}

vnode_t* vfs_create_new_vnode(const char* name, const struct stat* st)
{
    assert(name && st);
    vnode_t* newn = malloc(sizeof(vnode_t));
    if (!newn) return NULL;
    vfs_total_nodes++;
    newn->children = newn->next = newn->prev = NULL;
    newn->name = strdup(name);
    newn->st = *st;
    newn->lock = (atomic_flag)ATOMIC_FLAG_INIT;
    newn->flags = VNODE_INIT;
    newn->parent = NULL;
    newn->ref = VNODE_REF_INIT;
    return newn;
}

void vnode_delete_ref(vnode_ref_t* ref)
{
    if (!ref || !ref->ptr) return;
    ref_dec(&ref->ptr->ref);
    *ref = (vnode_ref_t){ NULL };
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

void vfs_add_new_child_node(vnode_ref_t node, const char* name, const struct stat* st)
{
    assert(node.ptr);

    vnode_t* child = vfs_create_new_vnode(name, st);

    uint32_t flags = acquire_spinlock_noint(&node.ptr->lock);

    child->parent = node.ptr;
    // parent -> child
    ref_inc(&child->ref);
    child->next = node.ptr->children;
    // do NOT count next/prev references to avoid race conditions
    if (node.ptr->children)
        node.ptr->children->prev = child;
    node.ptr->children = child;

    release_spinlock_noint(&node.ptr->lock, flags);
}

void vfs_unload_children(vnode_ref_t node)
{
    assert(node.ptr);
    uint32_t flags = acquire_spinlock_noint(&node.ptr->lock);
    vnode_t* child = node.ptr->children;
    node.ptr->children = NULL;
    release_spinlock_noint(&node.ptr->lock, flags);
    if (child)
        ref_dec(&child->ref);
}

void vnode_free(const struct ref* _ref)
{
    vnode_t* node = container_of(_ref, vnode_t, ref);
    if (node == vfs_root_node.ptr) return;
    uint32_t flags = acquire_spinlock_noint(&node->lock);
    if (!node->parent)
    {
        release_spinlock_noint(&node->lock, flags);
        return;
    }
    node->parent = NULL;
    release_spinlock_noint(&node->lock, flags);
    vnode_ref_t ref = { node };
    if (!node || !_ref)
        return;
    // LOG(TRACE, "Destroying inode %zu", (size_t)node->st.st_ino);
    vnode_ref_t child = vnode_dereference(ref, children);
    vnode_ref_t prev = vnode_dereference(ref, prev);
    vnode_ref_t next = vnode_dereference(ref, next);
    if (child.ptr)
        ref_dec(&child.ptr->ref);
    if (prev.ptr)
    {
        prev.ptr->next = NULL;
        ref_dec(&prev.ptr->ref);
    }
    if (next.ptr)
    {
        next.ptr->prev = NULL;
        ref_dec(&next.ptr->ref);
    }
    vnode_delete_ref(&child);
    vnode_delete_ref(&prev);
    vnode_delete_ref(&next);
    vfs_total_nodes--;
    free(node->name);
    free(node);
}

void vfs_log_structure_helper(vnode_ref_t node, int depth)
{
    assert(node.ptr);

    LOG(DEBUG, "%*s- \"%s\" (inode %ld)%s", depth, "", node.ptr->name, (long)node.ptr->st.st_ino, ((node.ptr->flags & VNODE_EXPLORED) || (!S_ISDIR(node.ptr->st.st_mode))) ? "" : " <NOT EXPLORED>");
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
    size_t offset = 0;
    vnode_ref_t parent_ref = vnode_dereference(ref, parent);
    if (parent_ref.ptr)
    {
        offset = vfs_get_absolute_path_to_node_helper(parent_ref, buf, bufsiz);
        vnode_delete_ref(&parent_ref);
        if (offset < bufsiz)
        {
            int maxwrite = bufsiz - offset;
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
    size_t nodes = vfs_count_nodes(vfs_root_node) + 1;
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

vnode_ref_t vfs_get_vnode_from_path(int* _errno, int uid, int gid, const char* path, vnode_ref_t cwd)
{
    assert(errno);
    *_errno = 0;
    while (*path == '/') path++;
    vnode_ref_t current = cwd.ptr ? cwd : vfs_root_node;
    vnode_ref_t child = vnode_dereference(current, children);
    size_t len = strlen(child.ptr->name);
    while (child.ptr)
    {
        if (strcmp_slash(child.ptr->name, path) == 0)
        {
            path += len;
            while (*path == '/')
                path++;
            vnode_delete_ref(&current);
            // TODO: Implement permission check
            if (!*path)
                return child;
            current = child;
            child = vnode_dereference(current, children);
            len = strlen(child.ptr->name);
            continue;
        }
        vnode_move_reference(&child, next);
    }
    vnode_delete_ref(&current);
    *_errno = ENOENT;
    return (vnode_ref_t){ NULL };
}
