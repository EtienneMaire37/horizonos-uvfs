#include "inode.h"
#include "ref.h"
#include <stdlib.h>
#include <assert.h>

inode_ref_t vfs_create_new_inode(const struct stat* st, void* fs_specific, void (*free_fs_specific_data)(void*))
{
    assert(st);
    inode_t* newn = calloc(1, sizeof(inode_t));
    if (!newn) return (inode_ref_t){ NULL };
    newn->st = *st;
    newn->st.st_nlink = 0;
    newn->lock = (atomic_flag)ATOMIC_FLAG_INIT;
    newn->fs_specific = fs_specific;
    newn->free_fs_specific_data = free_fs_specific_data;
    newn->ref = INODE_REF_INIT;
    return (inode_ref_t){ newn };
}

void ___inode_free(const struct ref* _ref)
{
    inode_t* inode = container_of(_ref, inode_t, ref);
    if (inode->free_fs_specific_data) inode->free_fs_specific_data(inode->fs_specific);
    free(inode);
}

void inode_delete_ref(inode_ref_t* ref)
{
    if (!ref || !ref->ptr) return;
    struct ref* ref_ref = ref->ptr ? &ref->ptr->ref : NULL;
    ref->ptr = NULL;
    if (ref_ref)
        ref_dec(ref_ref);
}

inode_ref_t inode_copy_ref(inode_ref_t ref)
{
    if (!ref.ptr) return ref;
    ref_inc(&ref.ptr->ref);
    return ref;
}
