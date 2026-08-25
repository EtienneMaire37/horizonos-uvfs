#include <linux/limits.h>
#include <sys/stat.h>
#include <assert.h>

#include "vnode.h"
#include "inode.h"
#include "fs/initrd.h"
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
    {
        vnode_ref_t dev_node = vfs_get_vnode_from_path(&errno, 0, 0, "/dev", (vnode_ref_t){ NULL }, (vnode_ref_t){ NULL }, false);
        if (errno)
        {
            perror("Couldn't find vnode for /dev");
            abort();
        }
        vfs_add_new_special_child_node(dev_node, "initrd", S_IFBLK | S_IRUSR | S_IRGRP | S_IROTH, 0, 0, initrd_read_device, initrd_write_device,
                                       initrd_open_device("./resources/initrd.tar"), initrd_close_device);
        vnode_delete_ref(&dev_node);
    }
    while (true)
    {
        fflush(stdout);
        char path[PATH_MAX], action[64];
        printf("Action? (\n\tstat: stat node, \n\ttree: get tree from node, \n\tunload: unload children, \n\tmount: mount filesystem, \n\tunmount: unmount filesystem, \n\tread: print file contents, \n\tmkdir: create folder) ");
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
        else if (strcmp(action, "unmount") == 0)
        {
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            vfs_unmount(node);
        }
        else if (strcmp(action, "read") == 0)
        {
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            uint8_t buf[BUFSIZ];
            off_t offset = 0;
            while ((errno = vnode_read(node, buf, sizeof(buf), offset)) > 0)
            {
                fwrite(buf, sizeof(buf), 1, stdout);
                offset += sizeof(buf);
            }
            errno *= -1;
            if (errno)
                perror("Couldn't read file");
        }
        else if (strcmp(action, "mkdir") == 0)
        {
            printf("Name of new dir? ");
            ret = get_input(action, sizeof(action));
            if (!ret)
            {
                perror("Couldn't read input");
                abort();
            }
            vnode_ref_t ref = vfs_get_vnode_from_path(&errno, 0, 0, path, vfs_root_node, (vnode_ref_t){ NULL }, true);
            if (!errno)
            {
                int _errno = vfs_mkdir(action, ref, 0775, 0, 0);
                vnode_delete_ref(&ref);
                if (_errno)
                {
                    errno = _errno;
                    perror("Couldn't create directory");
                }
            }
            else
                perror("Couldn't find parent");
        }
        else
            printf("Invalid action\n");
        vnode_delete_ref(&node);
        vfs_verify_tree_integrity();
    }
}
