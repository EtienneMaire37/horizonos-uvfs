#include "vnode.h"
#include "spinlock.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "log.h"

vnode_t* vfs_root_node = NULL;

vnode_t* vfs_create_new_vnode(const char* name, const struct stat* st)
{
    vnode_t* new = malloc(sizeof(vnode_t));
    if (!new) return NULL;
    new->children = new->next = new->prev = NULL;
    new->name = strdup(name);
    new->st = *st;
    new->lock = (atomic_flag)ATOMIC_FLAG_INIT;
    new->flags = VNODE_INIT;
    new->reference_count = 0;
    return new;
}

void vfs_add_new_node(vnode_t* node, vnode_t* new)
{
    assert(node && new);
    
    new->prev = node;
    new->reference_count++;
    
    uint32_t flags = acquire_spinlock_noint(&node->lock);

    vnode_t* prev_node = node;
    while (prev_node->next)
        prev_node = prev_node->next;

    prev_node->next = new;
    
    release_spinlock_noint(&node->lock, flags);
}

void __vfs_unload_children(vnode_t* node)
{
    assert(node);
    while (node->children)
    {
        node->children->reference_count--;
        vnode_t* next = node->children->next;
        if (node->children->reference_count <= 0)
        {
            __vfs_unload_children(node->children);
            vfs_node_destroy(node->children);
        }
        node->children = next;
    }
}

void vfs_node_destroy(vnode_t* node)
{
    assert(node);
    assert(node->reference_count <= 0);
    free(node);
}

void vfs_log_structure_helper(vnode_t* node, int depth)
{
    assert(node);

    LOG("%*s- \"%s\" (inode %ld)", depth, "", node->name, (long)node->st.st_ino);
    vnode_t* child = node->children;
    while (child)
    {
        vfs_log_structure_helper(child, depth + 4);
        child = child->next;
    }
}

void vfs_log_structure(vnode_t *node)
{
    vfs_log_structure_helper(node, 0);
}
