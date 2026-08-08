#pragma once

#include "../vnode_ref.h"
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

int posix_explore(vnode_ref_t ref);
ssize_t posix_read(vnode_ref_t ref, void* buf, size_t bytes, off_t offset);
ssize_t posix_write(vnode_ref_t ref, void* buf, size_t bytes, off_t offset);

