#include <sys/stat.h>
#include <assert.h>

#include "vnode.h"
#include "explore.h"

int main()
{
    vfs_create_root_node();
    vfs_explore(vfs_root_node);
    vfs_explore(vfs_root_node->children);
    // vfs_unload_children(vfs_root_node);
    vfs_log_structure((vnode_ref_t){ vfs_root_node });
    vfs_verify_tree_integrity();
}
