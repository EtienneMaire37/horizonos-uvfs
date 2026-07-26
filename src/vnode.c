#include "vnode.h"
#include "flags.h"
#include "spinlock.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

vnode_t* _Atomic vfs_root_node = NULL;
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

    st.st_ino = vfs_generate_ino();
    st.st_mode = S_IFDIR | S_IRWXO | S_IRWXG;

    st.st_nlink = 1;
    st.st_size = 0;

    vnode_t* node = vfs_create_new_vnode("/", &st);
    ref_inc(&node->ref);
    vfs_root_node = node;
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

void vfs_add_new_child_node(vnode_t* node, const char* name, const struct stat* st)
{
    assert(node);

    vnode_t* child = vfs_create_new_vnode(name, st);

    uint32_t flags = acquire_spinlock_noint(&node->lock);

    child->parent = node;
    ref_inc(&node->ref);
    child->next = node->children;
    if (node->children)
    {
        ref_inc(&node->children->ref);
        ref_inc(&node->ref);
        node->children->prev = child;
    }
    node->children = child;

    release_spinlock_noint(&node->lock, flags);
}

void vfs_unload_children(vnode_t* node)
{
    assert(node);
    uint32_t flags = acquire_spinlock_noint(&node->lock);
    vnode_t* child = node->children;
    node->children = NULL;
    release_spinlock_noint(&node->lock, flags);
    while (child)
    {
        vnode_t* next = child->next;
        ref_dec(&child->ref);
        if (next)
        {
            next->prev = NULL;
            ref_dec(&child->ref);
        }
        child = next;
    }
}

void vnode_free(const struct ref* ref)
{
    vnode_t* node = container_of(ref, vnode_t, ref);
    vnode_t* child = node->children;
    vnode_t* prev = node->prev;
    vnode_t* next = node->next;
    // Assume a parent is always referenced from higher
    vfs_total_nodes--;
    free(node->name);
    free(node);
    if (child)
        ref_dec(&child->ref);
    if (prev)
        ref_dec(&prev->ref);
    if (next)
        ref_dec(&next->ref);
}

void vfs_log_structure_helper(vnode_t* node, int depth)
{
    assert(node);

    LOG("%*s- \"%s\" (inode %ld)%s", depth, "", node->name, (long)node->st.st_ino, node->flags & VNODE_EXPLORED ? "" : " <NOT EXPLORED>");
    vnode_t* child = node->children;
    while (child)
    {
        vfs_log_structure_helper(child, depth + 4);
        child = child->next;
    }
}

void vfs_log_structure(vnode_t* node)
{
    vfs_log_structure_helper(node, 0);
}

size_t vfs_get_absolute_path_to_node_helper(vnode_t* node, char* buf, size_t bufsiz)
{
    assert(node);
    size_t offset = 0;
    uint32_t flags = acquire_spinlock_noint(&node->lock);
    if (node->parent)
    {
        offset = vfs_get_absolute_path_to_node_helper(node->parent, buf, bufsiz);
        if (offset < bufsiz)
            offset += snprintf(&buf[offset], bufsiz, "/%*s", (int)(bufsiz - offset), node->name);
    }
    release_spinlock_noint(&node->lock, flags);
    return offset;
}

void vfs_get_absolute_path_to_node(vnode_t* node, char* buf, size_t bufsiz)
{
    assert(bufsiz > 0);
    assert(node);
    if (node->parent)
        buf[vfs_get_absolute_path_to_node_helper(node, buf, bufsiz - 1)] = 0;
    else
        strcpy(buf, "/");
}

size_t vfs_count_nodes(vnode_t* node)
{
    assert(node);
    size_t total = 0;
    uint32_t flags = acquire_spinlock_noint(&node->lock);
    vnode_t* child = node->children;
    while (child)
    {
        total++;
        total += vfs_count_nodes(child);
        child = child->next;
    }
    release_spinlock_noint(&node->lock, flags);
    return total;
}

bool vfs_verify_tree_integrity()
{
    size_t nodes = vfs_count_nodes(vfs_root_node) + 1;
    bool ret = nodes == vfs_total_nodes;
    #ifndef NDEBUG
    if (!ret)
    {
        LOG("Total refcounted nodes: %zu", vfs_total_nodes);
        LOG("Total nodes in tree: %zu", nodes);
        abort();
    }
    #endif
    return ret;
}

ino_t vfs_generate_ino()
{
    static ino_t ino = 1;
    return ino++;
}
