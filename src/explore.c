#include "explore.h"
#include "flags.h"
#include "vnode.h"
#include "spinlock.h"
#include "inode.h"
#include "mountpoint.h"

#include <assert.h>
#include <linux/limits.h>
#include <sys/dir.h>
#include <errno.h>

int vfs_explore(vnode_ref_t ref)
{
    vnode_t* node = ref.ptr;
    if (!node) return ENOENT;
    uint32_t flags = acquire_spinlock_noint(&node->lock);
    if ((node->flags & VNODE_EXPLORED) || (node->flags & VNODE_EXPLORING) || !S_ISDIR(node->inode.ptr->st.st_mode))
    {
        release_spinlock_noint(&node->lock, flags);
        return 0;
    }
    node->flags |= VNODE_EXPLORING;
    release_spinlock_noint(&node->lock, flags);
    vfs_unload_children(ref);

    int ret = ENOSYS; 
    if (!node->mountpoint.ptr)
    // NOTE: Don't log anything because it is a normal occurence with virtfs mountpoints
        ; // LOG(WARN, "NULL mountpoint");
    else
    {
        int (*explore)(vnode_ref_t) = node->mountpoint.ptr->explore;
        ret = explore ? explore(ref) : 0;
    }

    flags = acquire_spinlock_noint(&node->lock);
    node->flags |= VNODE_EXPLORED;
    node->flags &= ~VNODE_EXPLORING;
    release_spinlock_noint(&node->lock, flags);

    return ret;
}
