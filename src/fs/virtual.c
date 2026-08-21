#include "virtual.h"
#include <errno.h>

ino_t virtfs_generate_ino()
{
    static ino_t num = 1;
    return num++;
}

int virtfs_explore(vnode_ref_t vnode)
{
    (void)vnode;
    return 0;
}
ssize_t virtfs_read(vnode_ref_t vnode, void* buf, size_t count, off_t offset)
{
    (void)vnode;
    (void)buf;
    (void)count;
    (void)offset;
    return -ENOSYS;
}
ssize_t virtfs_write(vnode_ref_t vnode, void* buf, size_t count, off_t offset)
{
    (void)vnode;
    (void)buf;
    (void)count;
    (void)offset;
    return -ENOSYS;
}
