#include "virtual.h"
#include "../inode.h"
#include "../mountpoint.h"
#include "../log.h"
#include <errno.h>
#include <limits.h>

ino_t virtfs_generate_ino()
{
    static ino_t num = 1;
    return num++;
}

int virtfs_explore(vnode_ref_t vnode)
{
    char path[PATH_MAX];
    vfs_get_relative_path_to_node_from_mountpoint(vnode, path, sizeof(path));
    LOG(DEBUG, "virtfs_explore: \"%s\"", path);
    int _errno;
    vnode_ref_t node = vfs_get_vnode_from_path(&_errno, 0, 0, path, (vnode_ref_t){ vnode.ptr->mountpoint.ptr->data }, (vnode_ref_t){ NULL }, false);
    if (!node.ptr) return _errno;
    vnode_move_reference(&node, children);
    while (node.ptr)
    {
        vfs_add_new_child_node__hardlink(vnode, node.ptr->name, node.ptr->inode);
        vnode_move_reference(&node, next);
    }
    
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
int virtfs_mkdir(const char* name, vnode_ref_t parent, struct stat* st)
{
    // TODO: Create new vnode in tree
    (void)name;
    (void)parent;
    (void)st;
    return 0;
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
