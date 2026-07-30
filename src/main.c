#include <sys/stat.h>
#include <assert.h>

#include "vnode.h"
#include "explore.h"

int main()
{
    vfs_create_root_node();
    vfs_explore(vfs_root_node);
    vnode_ref_t child = vnode_dereference(vfs_root_node, children);
    vfs_explore(child);
    vnode_delete_ref(&child);
    // vfs_unload_children(vfs_root_node);
    vfs_log_structure(vfs_root_node);
    vfs_verify_tree_integrity();
}
