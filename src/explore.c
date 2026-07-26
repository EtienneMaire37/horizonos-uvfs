#include "explore.h"
#include "flags.h"
#include "vnode.h"
#include "spinlock.h"

#include <assert.h>
#include <linux/limits.h>
#include <sys/dir.h>

void vfs_explore(vnode_t* node)
{
    assert(node);
    uint32_t flags = acquire_spinlock_noint(&node->lock);
    if (node->flags & VNODE_EXPLORED)
    {
        release_spinlock_noint(&node->lock, flags);
        return;
    }
    release_spinlock_noint(&node->lock, flags);
    vfs_unload_children(node);

    {
        char path[PATH_MAX];
        vfs_get_absolute_path_to_node(node, path, sizeof(path));
        DIR* dir = opendir(path);
        if (dir)
        {
            closedir(dir);
        }
    }

    flags = acquire_spinlock_noint(&node->lock);
    node->flags |= VNODE_EXPLORED;
    release_spinlock_noint(&node->lock, flags);
}
