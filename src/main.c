#include <linux/limits.h>
#include <sys/stat.h>
#include <assert.h>

#include "vnode.h"
#include <stdio.h>
#include <errno.h>

int main()
{
    vfs_create_root_node();
    while (true)
    {
        printf("File to stat: ");
        fflush(stdout);
        char buf[PATH_MAX];
        char* ret = fgets(buf, sizeof(buf) - 1, stdin);
        if (!ret)
        {
            perror("Couldn't read input");
            return 1;
        }
        char ch;
        size_t i = 0;
        while ((ch = ret[i]))
        {
            if (ch == '\n')
                ret[i] = 0;
            i++;
        }
        int _errno;
        vnode_ref_t node = vfs_get_vnode_from_path(&_errno, 0, 0, buf, (vnode_ref_t){ NULL });
        errno = _errno;
        if (!node.ptr)
            perror("Couldn't read vnode");
        else
        {
            printf("Mode: %#o\n", node.ptr->st.st_mode);
            vnode_delete_ref(&node);
            printf("Tree:\n");
            vfs_log_structure(vfs_root_node);
            vfs_verify_tree_integrity();
        }
    }
}
