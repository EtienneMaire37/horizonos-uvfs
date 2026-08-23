#include "initrd.h"
#include <errno.h>

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

ssize_t initrd_read_device(vnode_ref_t vnode, void* buf, size_t count, off_t offset)
{
    (void)vnode;
    (void)buf;
    (void)count;
    (void)offset;
    return -ENOSYS;
}
ssize_t initrd_write_device(vnode_ref_t vnode, void* buf, size_t count, off_t offset)
{
    
    (void)vnode;
    (void)buf;
    (void)count;
    (void)offset;
    return -EROFS;
}
