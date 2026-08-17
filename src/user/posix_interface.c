#include "../vnode.h"
#include "../log.h"
#include <limits.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/dir.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int posix_explore(vnode_ref_t ref)
{
    char path[PATH_MAX];
    char current_path[PATH_MAX];
    size_t len = vfs_get_absolute_path_to_node(ref, path, sizeof(path));
    memcpy(current_path, path, PATH_MAX);

    LOG(TRACE, "vfs_explore: Exploring path \"%s\"", path);
    DIR *dir = opendir(path);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir))) {
            if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
                sprintf(&current_path[len], "%s", ent->d_name);
                current_path[len - 1] = '/';
                struct stat st;
                if (lstat(current_path, &st) == 0)
                {
                    LOG(TRACE, "Adding node \"%s\" to \"%s\"", ent->d_name, path);
                    vfs_add_new_child_node(ref, ent->d_name, &st);
                }
                else {
                    char buf[PATH_MAX + 17];
                    snprintf(buf, sizeof(buf) - 1, "Couldn't stat \"%s\"", current_path);
                    perror(buf);
                }
            }
        }
        closedir(dir);
    }
    LOG(TRACE, "vfs_explore: finished exploring path.");
    return 0;
}

ssize_t posix_read(vnode_ref_t ref, void* buf, size_t bytes, off_t offset)
{
    char path[PATH_MAX];
    vfs_get_absolute_path_to_node(ref, path, sizeof(path));

    struct stat st;
    if (lstat(path, &st) != 0)
    {
        perror("posix_read: lstat");
        return -errno;
    }

    if (S_ISLNK(st.st_mode))
    {
        ssize_t ret = readlink(path, buf, bytes);
        if (ret == -1)
        {
            perror("posix_read: readlink");
            return -errno;
        }
        ((char*)buf)[bytes - 1] = 0;
        return ret;
    }
   
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd == -1)
    {
        perror("posix_read: open");
        return -errno;
    }
    size_t ret = read(fd, buf, bytes);
    if (ret == -1)
    {
        ret = -errno;
        perror("posix_read: read");
    }
    close(fd);
    return ret;
}

ssize_t posix_write(vnode_ref_t ref, void* buf, size_t bytes, off_t offset)
{
    return -EROFS;
    // char path[PATH_MAX];
    // vfs_get_absolute_path_to_node(ref, path, sizeof(path));

    // struct stat st;
    // if (lstat(path, &st) != 0)
    // {
    //     perror("posix_write: lstat");
    //     return -errno;
    // }

    // if (S_ISLNK(st.st_mode))
    // {
    //     ((char*)buf)[bytes - 1] = 0;
    //     ssize_t ret = symlink(buf, path);
    //     if (ret == -1)
    //     {
    //         perror("posix_write: symlink");
    //         return -errno;
    //     }
    //     return ret;
    // }
   
    // int fd = open(path, O_WRONLY | O_NOFOLLOW);
    // if (fd == -1)
    // {
    //     perror("posix_write: open");
    //     return -errno;
    // }
    // size_t ret = write(fd, buf, bytes);
    // if (ret == -1)
    // {
    //     ret = -errno;
    //     perror("posix_write: write");
    // }
    // close(fd);
    // return ret;
}
