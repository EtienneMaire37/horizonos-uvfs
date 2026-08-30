#pragma once

#include <sys/stat.h>
#include "../vnode.h"

ino_t virtfs_generate_ino();
int virtfs_explore(vnode_ref_t vnode);
ssize_t virtfs_read(vnode_ref_t vnode, void* buf, size_t count, off_t offset);
ssize_t virtfs_write(vnode_ref_t vnode, void* buf, size_t count, off_t offset);
void virtfs_flush(vnode_ref_t vnode);
