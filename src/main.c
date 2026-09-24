#include <linux/limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "vnode.h"
#include "inode.h"
#include "log.h"
#include "explore.h"
#include "fs/initrd.h"
#include "fs/null.h"
#include "util/assert.h"
#include "open_file.h"

static inline char* get_input(char* buf, size_t bytes)
{
    fflush(stdout);
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

#define get_node() \
        printf("Path to node? "); \
        ret = get_input(path, sizeof(path)); \
        if (!ret) \
        { \
            perror("Couldn't read input"); \
            abort(); \
        } \
        int _errno; \
        vnode_ref_t node = vfs_get_vnode_from_path(&_errno, 0, 0, path, vfs_root_node, (vnode_ref_t){ NULL }, true);

#define check_input(ret) \
        do { if (!ret) \
        { \
            perror("Couldn't read input"); \
            abort(); \
        } } while (0)

int main()
{
    ASSERT(sizeof(vnode_ref_t) == sizeof(uintptr_t));
    initrd_init("./resources/initrd.tar");
    vfs_create_root_node();
    LOG(DEBUG, "Mounting initrd at root");
    errno = vfs_mount(vfs_root_node, (vnode_ref_t){ NULL }, "virt");
    if (errno)
        perror("Couldn't mount root");
    LOG(DEBUG, "Creating /dev");
    errno = vfs_create("dev", vfs_root_node, 0755 | S_IFDIR, 0, 0);
    LOG(TRACE, "Done");
    if (errno)
        perror("Couldn't create /dev");
    {
        vnode_ref_t dev_node = vfs_get_vnode_from_path(&errno, 0, 0, "/dev", (vnode_ref_t){ NULL }, (vnode_ref_t){ NULL }, false);
        if (errno)
        {
            perror("Couldn't find vnode for /dev");
            abort();
        }

        vfs_create("null", dev_node, S_IFCHR | 0666, 0, 0, null_read, null_write);
        vfs_create("tty", dev_node, S_IFCHR | 0666, 0, 0, NULL, NULL);
        vfs_create("console", dev_node, S_IFCHR | 0666, 0, 0, NULL, NULL);
        
        vnode_delete_ref(&dev_node);
    }
    LOG(DEBUG, "Creating /tmp");
    errno = vfs_create("tmp", vfs_root_node, 01777 | S_IFDIR, 0, 0);
    if (errno)
        perror("Couldn't create /tmp");
    while (true)
    {
        char path[PATH_MAX], action[64];
        printf("Action? (\n\tstat: stat node,\n\ttree: get tree from node,\n\tunload: unload children,\n\tmount: mount filesystem,\n\tunmount: unmount filesystem,\n\tread: print file contents,\n\tcreate: create empty file,\n\tmkdir: create folder,\n\texplore: explore folder,\n\tfds: list open file descriptors,\n\topen: open a new open file descriptor pointing to a file,\n\tclose: close a file descriptor) ");
        char* ret = get_input(action, sizeof(action));
        check_input(ret);
        if (strcmp(action, "stat") == 0)
        {
            get_node()
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            printf("Inode: %lu\n", (unsigned long)node.ptr->inode.ptr->st.st_ino);
            printf("Mode: %#o\n", (unsigned int)node.ptr->inode.ptr->st.st_mode);
            printf("Uid: %u\tGid: %u\n", (unsigned int)node.ptr->inode.ptr->st.st_uid, (unsigned int)node.ptr->inode.ptr->st.st_gid);
            vnode_delete_ref(&node);
        }
        else if (strcmp(action, "tree") == 0)
        {
            get_node()
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            printf("Tree:\n");
            vfs_log_structure(node);
            vnode_delete_ref(&node);
        }
        else if (strcmp(action, "unload") == 0)
        {
            get_node()
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            vfs_unload_children(node);
            vnode_delete_ref(&node);
        }
        else if (strcmp(action, "mount") == 0)
        {
            get_node()
            printf("Type of file system to mount? (virt: virtual (in memory) file system, initrd: ustar file containing the initrd) ");
            ret = get_input(action, sizeof(action));
            check_input(ret);
            printf("Path to the device to mount? ");
            ret = get_input(path, sizeof(path));
            check_input(ret);
            vnode_ref_t mount_device = vfs_get_vnode_from_path(&_errno, 0, 0, path, vfs_root_node, (vnode_ref_t){ NULL }, true);
            errno = vfs_mount(node, mount_device, action);
            vnode_delete_ref(&mount_device);
            if (errno)
                perror("Couldn't mount");
            vnode_delete_ref(&node);
        }
        else if (strcmp(action, "unmount") == 0)
        {
            get_node()
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            if ((errno = vfs_unmount(node)))
                perror("Couldn't unmount node");
            vnode_delete_ref(&node);
        }
        else if (strcmp(action, "read") == 0)
        {
            get_node()
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
            vnode_delete_ref(&node);
        }
        else if (strcmp(action, "mkdir") == 0 || strcmp(action, "create") == 0)
        {
            get_node()
            bool file = strcmp(action, "create") == 0;
            const char* valname = file ? "file" : "directory";
            printf("Name of new %s? ", valname);
            ret = get_input(action, sizeof(action));
            check_input(ret);
            _errno = vfs_create(action, node, 0775 | (file ? S_IFREG : S_IFDIR), 0, 0);
            if (_errno)
            {
                errno = _errno;
                perror("Couldn't create directory entry");
            }
            vnode_delete_ref(&node);
        }
        else if (strcmp(action, "explore") == 0)
        {
            get_node()
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            vfs_explore(node);
            vnode_delete_ref(&node);
        }
        else if (strcmp(action, "open") == 0)
        {
            get_node()
            if (!node.ptr)
            {
                errno = _errno;
                perror("Couldn't read vnode");
                continue;                    
            }
            int flags;
            printf("Flags? ");
            ret = get_input(action, sizeof(action));
            check_input(ret);
            flags = strtol(ret, NULL, 0);
            mode_t mode;
            printf("Mode? ");
            ret = get_input(action, sizeof(action));
            check_input(ret);
            mode = strtol(ret, NULL, 0);
            errno = -vfs_open(path, flags, mode, 0, 0, vfs_root_node, vfs_root_node, S_IWGRP | S_IWOTH);
            vnode_delete_ref(&node);
            if (errno > 0)
                perror("Couldn't open file");
        }
        else if (strcmp(action, "close") == 0)
        {
            printf("fd to close? ");
            ret = get_input(action, sizeof(action));
            check_input(ret);
            errno = vfs_close(atoi(action));
            if (errno)
                perror("Couldn't close fd");
        }
        else if (strcmp(action, "fds") == 0)
        {
            vfs_log_thread_fds();
        }
        else
            printf("Invalid action\n");
        vfs_verify_tree_integrity();
    }
}
