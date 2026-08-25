#pragma once

#include <sys/stat.h>
#include "../vnode.h"

ino_t initrd_generate_ino();
int initrd_explore(vnode_ref_t vnode);
ssize_t initrd_read(vnode_ref_t vnode, void* buf, size_t count, off_t offset);
ssize_t initrd_write(vnode_ref_t vnode, void* buf, size_t count, off_t offset);
int* initrd_open_device(const char* path);
void initrd_close_device(inode_t* inode);
ssize_t initrd_read_device(vnode_ref_t vnode, void* buf, size_t count, off_t offset);
ssize_t initrd_write_device(vnode_ref_t vnode, void* buf, size_t count, off_t offset);

