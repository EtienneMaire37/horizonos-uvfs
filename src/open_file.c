#include "open_file.h"
#include "ref.h"
#include <stdatomic.h>
#include <stdlib.h>

open_file_descriptor_ref_t vfs_allocate_new_open_file_descriptor(int flags, vnode_ref_t vnode, struct stat *st)
{
    open_file_descriptor_ref_t ref = { malloc(sizeof(open_file_descriptor_t)) };
    if (!ref.ptr) return ref;
    ref.ptr->ref = OPEN_FD_REF_INIT;
    ref_inc(&ref.ptr->ref);
    ref.ptr->st = *st;
    ref.ptr->vnode = vnode;
    ref.ptr->lock = (atomic_flag)ATOMIC_FLAG_INIT;
    ref.ptr->flags = flags;
    ref.ptr->offset = 0;
    return ref;
}

void open_file_descriptor_free(const struct ref *ref)
{
    free(container_of(ref, open_file_descriptor_t, ref));
}
