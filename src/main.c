#include <linux/limits.h>
#include <sys/stat.h>
#include <assert.h>

#include "vnode.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main()
{
    assert(sizeof(vnode_ref_t) == sizeof(uintptr_t));
    vfs_create_root_node();
    while (true)
    {
        printf("Path to file: ");
        fflush(stdout);
        char path[PATH_MAX];
        char* retp = fgets(path, sizeof(path) - 1, stdin);
        if (!retp)
        {
            perror("Couldn't read input");
            return 1;
        }
        char ch;
        size_t i = 0;
        while ((ch = path[i]))
        {
            if (ch == '\n')
                path[i] = 0;
            i++;
        }
        int _errno;
        vnode_ref_t node = vfs_get_vnode_from_path(&_errno, 0, 0, path, (vnode_ref_t){ NULL }, true);
        errno = _errno;
        if (!node.ptr)
            perror("Couldn't read vnode");
        else
        {
            printf("Action? (stat: stat node, tree: get tree from node, unload: unload children) ");
            fflush(stdout);
            char action[64];
            char* reta = fgets(action, sizeof(action) - 1, stdin);
            i = 0;
            while ((ch = action[i]))
            {
                if (ch == '\n')
                    action[i] = 0;
                i++;
            }
            if (strcmp(action, "stat") == 0)
            {
                printf("Inode: %lu\n", (unsigned long)node.ptr->st.st_ino);
                printf("Mode: %#o\n", (unsigned int)node.ptr->st.st_mode);
                printf("Uid: %u\tGid: %u\n", (unsigned int)node.ptr->st.st_uid, (unsigned int)node.ptr->st.st_gid);
            }
            else if (strcmp(action, "tree") == 0)
            {
                printf("Tree:\n");
                vfs_log_structure(node);
            }
            else if (strcmp(action, "unload") == 0)
            {
                vfs_unload_children(node);
            }
            else
                printf("Invalid action\n");
            vnode_delete_ref(&node);
            vfs_verify_tree_integrity();
        }
    }
}
