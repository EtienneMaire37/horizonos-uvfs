#include "virtual.h"
#include "../inode.h"
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

void virtfs_flush(vnode_ref_t vnode)
{
    (void)vnode;
}
vnode_t* virtfs_create_data()
{
    struct stat st = {0};
    st.st_mode = S_IFDIR;

    inode_ref_t inode = vfs_create_new_inode(&st, NULL, NULL);
    vnode_ref_t ref = vfs_create_new_vnode("virtfs_root", inode);
    inode_delete_ref(&inode);
    return ref.ptr;
}
void virtfs_free_data(void* node_ref)
{
    vnode_ref_t ref = { node_ref };
    vfs_unload_children(ref);
    vnode_delete_ref(&ref);
}
