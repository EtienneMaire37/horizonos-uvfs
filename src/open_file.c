#include "open_file.h"
#include "ref.h"
#include "vnode.h"
#include "vnode_ref.h"
#include "spinlock.h"
#include "util/assert.h"
#include <stdlib.h>

/* STUB */
// TODO: Use rw locks everywhere it should be used
static spinlock_noint_t lock;
#define OFDT_ENTRIES   64
static open_file_descriptor_entry_t entries[OFDT_ENTRIES];
/* STUB */

void open_file_descriptor_delete_ref(open_file_descriptor_ref_t* ref)
{
    if (!ref || !ref->ptr) return;
    struct ref* ref_ref = ref->ptr ? &ref->ptr->ref : NULL;
    ref->ptr = NULL;
    if (ref_ref)
        ref_dec(ref_ref);
}

open_file_descriptor_ref_t open_file_descriptor_copy_ref(open_file_descriptor_ref_t ref)
{
    if (!ref.ptr) return ref;
    ref_inc(&ref.ptr->ref);
    return ref;
}

open_file_descriptor_ref_t vfs_allocate_new_open_file_descriptor(int flags, vnode_ref_t vnode, struct stat *st)
{
    open_file_descriptor_ref_t ref = { malloc(sizeof(open_file_descriptor_t)) };
    if (!ref.ptr) return ref;
    ref.ptr->ref = OPEN_FD_REF_INIT;
    ref_inc(&ref.ptr->ref);
    ref.ptr->st = *st;
    ref.ptr->vnode = vnode_copy_ref(vnode);
    ref.ptr->lock = SPINLOCK_NOINT_INIT;
    ref.ptr->flags = flags;
    ref.ptr->offset = 0;
    return ref;
}

void ___open_file_descriptor_free(const struct ref *ref)
{
    open_file_descriptor_t* ofd = container_of(ref, open_file_descriptor_t, ref);
    vnode_delete_ref(&ofd->vnode);
    free(ofd);
}

int vfs_allocate_thread_fd(open_file_descriptor_ref_t desc, int flags)
{
    int fd = -1;
    uint32_t eflags = acquire_spinlock_noint(&lock);
    for (int i = 0; i < OFDT_ENTRIES; i++)
    {
        if (entries[i].fd.ptr == NULL)
        {
            entries[i].fd.ptr = open_file_descriptor_copy_ref(desc).ptr;
            entries[i].flags = flags;
            goto end;
        }
    }
end:
    release_spinlock_noint(&lock, eflags);
    return fd;
}
bool vfs_get_thread_fd(int fd, open_file_descriptor_ref_t* desc, int* flags)
{
    ASSERT(desc);
    ASSERT(flags);
    if (fd < 0 || fd >= OFDT_ENTRIES) return false;
    uint32_t eflags = acquire_spinlock_noint(&lock);
    *desc = open_file_descriptor_copy_ref(entries[fd].fd);
    *flags = entries[fd].flags;
    release_spinlock_noint(&lock, eflags);
    return true;
}
bool vfs_close_thread_fd(int fd)
{
    if (fd < 0 || fd >= OFDT_ENTRIES) return false;
    uint32_t eflags = acquire_spinlock_noint(&lock);
    open_file_descriptor_delete_ref(&entries[fd].fd);
    release_spinlock_noint(&lock, eflags);
    return true;
}
