#include "mountpoint.h"
#include "mountpoint_ref.h"
#include "ref.h"
#include "vnode.h"
#include <stdlib.h>
#include <assert.h>

void mountpoint_delete_ref(mountpoint_ref_t* ref)
{
    assert(ref);
    if (ref->ptr)
        ref_dec(&ref->ptr->ref);
}

void ___mountpoint_free(const struct ref* ref)
{
    mountpoint_t* mountpoint = container_of(ref, mountpoint_t, ref);
    vnode_delete_ref(&mountpoint->root);
    free(mountpoint);
}
