#include <sys/stat.h>

#include "vnode.h"
#include "explore.h"

int main()
{
    struct stat st;
    st.st_atim = (struct timespec){0, 0};
    st.st_ctim = (struct timespec){0, 0};
    st.st_mtim = (struct timespec){0, 0};
    st.st_blksize = 4096;
    st.st_blocks = 0;
    st.st_dev = 0;
    st.st_rdev = 0;

    st.st_gid = 0;
    st.st_uid = 0;

    st.st_ino = 1;
    st.st_mode = S_IFDIR | S_IRWXO | S_IRWXG;

    st.st_nlink = 1;
    st.st_size = 0;
    
    vfs_root_node = vfs_create_new_vnode("/", &st);
    vfs_explore(vfs_root_node);

    vfs_log_structure(vfs_root_node);
}
