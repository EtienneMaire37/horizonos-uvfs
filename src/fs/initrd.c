#include "initrd.h"
#include "../inode.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

ino_t initrd_generate_ino()
{
    static ino_t num = 1;
    return num++;
}

int initrd_explore(vnode_ref_t vnode)
{
    (void)vnode;
    return ENOSYS;
}
ssize_t initrd_read(vnode_ref_t vnode, void* buf, size_t count, off_t offset)
{
    (void)vnode;
    (void)buf;
    (void)count;
    (void)offset;
    return -ENOSYS;
}
ssize_t initrd_write(vnode_ref_t vnode, void* buf, size_t count, off_t offset)
{
    (void)vnode;
    (void)buf;
    (void)count;
    (void)offset;
    return -EROFS;
}
int* initrd_open_device(const char* path)
{
    int* ptr = malloc(sizeof(int));
    if (!ptr) return NULL;
    *ptr = open(path, O_RDONLY);
    assert(*ptr != -1);
    return ptr;
}
void initrd_close_device(inode_t* inode)
{
    close(*(int*)inode->fs_specific);
    free(inode->fs_specific);
}
ssize_t initrd_read_device(vnode_ref_t vnode, void* buf, size_t count, off_t offset)
{
    int fd = *(int*)vnode.ptr->inode.ptr->fs_specific;
    lseek(fd, offset, SEEK_SET);
    ssize_t ret = read(fd, buf, count);
    return ret >= 0 ? ret : -errno;
}
ssize_t initrd_write_device(vnode_ref_t vnode, void* buf, size_t count, off_t offset)
{
    (void)vnode;
    (void)buf;
    (void)count;
    (void)offset;
    return -EROFS;
}
