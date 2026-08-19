#pragma once

#include "mountpoint_ref.h"
#include "vnode_ref.h"
#include "ref.h"
#include <sys/stat.h>

struct mountpoint
{
    struct ref ref;
    
    vnode_ref_t root;
    dev_t dev;

    ino_t (*generate_ino)();
};

#define MOUNTPOINT_REF_INIT ((struct ref){ .count = 1, .free = ___mountpoint_free })

typedef enum
{
    FSTYPE_VIRTUAL,
    FSTYPE_INITRD,
    FSTYPE_UNKNOWN
} fstype_t;

void mountpoint_delete_ref(mountpoint_ref_t* ref);
void ___mountpoint_free(const struct ref* ref);
