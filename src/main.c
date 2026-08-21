#include <linux/limits.h>
#include <sys/stat.h>
#include <assert.h>

#include "vnode.h"
#include "inode.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static inline char* get_input(char* buf, size_t bytes)
{
    char* ret = fgets(buf, bytes - 1, stdin);
    if (!ret) return NULL;
    ret[bytes - 1] = 0;
    size_t i = 0;
    char ch;
    while ((ch = ret[i]))
    {
        if (ch == '\n')
        {
            ret[i] = 0;
            break;
        }
        i++;
    }
    return ret;
}

int main()
{
    assert(sizeof(vnode_ref_t) == sizeof(uintptr_t));
    vfs_create_root_node();
    errno = vfs_mount(vfs_root_node, (vnode_ref_t){ NULL }, "virt");
    if (errno)
        perror("Couldn't mount root");
    errno = vfs_mkdir("dev", vfs_root_node, 0755, 0, 0);
    if (errno)
        perror("Couldn't create /dev");
    while (true)
    {
        fflush(stdout);
        char path[PATH_MAX], action[64];
        printf("Action? (stat: stat node, tree: get tree from node, unload: unload children, mount: mount filesystem) ");
        fflush(stdout);
        char* ret = get_input(action, sizeof(action));
        if (!ret)
        {
            perror("Couldn't read input");
            abort();
        }
        printf("Path to node? ");
        fflush(stdout);
        ret = get_input(path, sizeof(path));
        if (!ret)
        {
            perror("Couldn't read input");
            abort();
        }

        int _errno;
        vnode_ref_t node = vfs_get_vnode_from_path(&_errno, 0, 0, path, vfs_root_node, (vnode_ref_t){ NULL }, true);
        if (strcmp(action, "stat") == 0)
        {
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            printf("Inode: %lu\n", (unsigned long)node.ptr->inode.ptr->st.st_ino);
            printf("Mode: %#o\n", (unsigned int)node.ptr->inode.ptr->st.st_mode);
            printf("Uid: %u\tGid: %u\n", (unsigned int)node.ptr->inode.ptr->st.st_uid, (unsigned int)node.ptr->inode.ptr->st.st_gid);
        }
        else if (strcmp(action, "tree") == 0)
        {
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            printf("Tree:\n");
            vfs_log_structure(node);
        }
        else if (strcmp(action, "unload") == 0)
        {
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            vfs_unload_children(node);
        }
        else if (strcmp(action, "mount") == 0)
        {
            printf("Type of file system to mount? (virt: virtual (in memory) file system, initrd: ustar file containing the initrd) ");
            ret = get_input(action, sizeof(action));
            if (!ret)
            {
                perror("Couldn't read input");
                abort();
            }
            printf("Path to the device to mount? ");
            ret = get_input(path, sizeof(path));
            if (!ret)
            {
                perror("Couldn't read input");
                abort();
            }
            vnode_ref_t mount_device = vfs_get_vnode_from_path(&_errno, 0, 0, path, vfs_root_node, (vnode_ref_t){ NULL }, true);
            errno = vfs_mount(node, mount_device, action);
            vnode_delete_ref(&mount_device);
            if (errno)
                perror("Couldn't mount");
        }
        else
            printf("Invalid action\n");
        if (node.ptr)
            vnode_delete_ref(&node);
        vfs_verify_tree_integrity();
    }
}
