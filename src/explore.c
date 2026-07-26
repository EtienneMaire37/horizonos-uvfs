#include "explore.h"
#include "flags.h"
#include "vnode.h"
#include "spinlock.h"
#include "log.h"

#include <assert.h>
#include <linux/limits.h>
#include <sys/dir.h>
#include <string.h>

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
        LOG("vfs_explore: Exploring path \"%s\"", path);
        DIR* dir = opendir(path);
        if (dir)
        {
            struct dirent* ent;
            while ((ent = readdir(dir)))
            {
                if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
                {
                    struct stat st;
                    st.st_ino = vfs_generate_ino();
                    vfs_add_new_child_node(node, ent->d_name, &st);
                }
            }
            closedir(dir);
        }
    }

    flags = acquire_spinlock_noint(&node->lock);
    node->flags |= VNODE_EXPLORED;
    release_spinlock_noint(&node->lock, flags);
}
