#include "explore.h"
#include "flags.h"
#include "vnode.h"
#include "spinlock.h"

#include <assert.h>

void vfs_explore(vnode_t* node)
{
    assert(node);
    if (node->flags & VNODE_EXPLORED) return;
    uint32_t flags = acquire_spinlock_noint(&node->lock);
    __vfs_unload_children(node);
    // TODO: Actually load nodes
    node->flags |= VNODE_EXPLORED;
    release_spinlock_noint(&node->lock, flags);
}
