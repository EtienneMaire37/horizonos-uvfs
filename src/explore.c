#include "explore.h"
#include "flags.h"
#include "vnode.h"
#include "spinlock.h"
#include "log.h"

#include <assert.h>
#include <linux/limits.h>
#include <sys/dir.h>
#include <string.h>

void vfs_explore(vnode_ref_t ref)
{
    vnode_t* node = ref.ptr;
    if (!node) return;
    uint32_t flags = acquire_spinlock_noint(&node->lock);
    if ((node->flags & VNODE_EXPLORED) || !S_ISDIR(node->st.st_mode))
    {
        release_spinlock_noint(&node->lock, flags);
        return;
    }
    release_spinlock_noint(&node->lock, flags);
    vfs_unload_children(ref);

    {
        char path[PATH_MAX];
        char current_path[PATH_MAX];
        size_t len = vfs_get_absolute_path_to_node(ref, path, sizeof(path));
        memcpy(current_path, path, PATH_MAX);

        LOG(TRACE, "vfs_explore: Exploring path \"%s\"", path);
        DIR* dir = opendir(path);
        if (dir)
        {
            struct dirent* ent;
            while ((ent = readdir(dir)))
            {
                if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
                {
                    sprintf(&current_path[len], "%s", ent->d_name);
                    current_path[len - 1] = '/';
                    struct stat st;
                    if (stat(current_path, &st) == 0)
                        vfs_add_new_child_node(ref, ent->d_name, &st);
                }
            }
            closedir(dir);
        }
    }

    flags = acquire_spinlock_noint(&node->lock);
    node->flags |= VNODE_EXPLORED;
    release_spinlock_noint(&node->lock, flags);
}
