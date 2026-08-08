#pragma once

#include <stddef.h>
#include "vnode.h"
#include "spinlock.h"

typedef struct vnode_ll_item vnode_ll_item_t;

struct vnode_ll_item
{
    vnode_t* node;
    const char* key;
    vnode_ll_item_t* next;
};

typedef struct
{
    size_t entries;
    vnode_ll_item_t** data;
    atomic_flag lock;
} vnode_hashmap_t;

vnode_hashmap_t* vnode_hashmap_create(size_t mem_limit);
void vnode_hashmap_destroy(vnode_hashmap_t** hmap);

vnode_t* vnode_hashmap_put(vnode_hashmap_t* hmap, const char* key, vnode_t* node);
vnode_t* vnode_hashmap_set(vnode_hashmap_t* hmap, const char* key, vnode_t* node);
void vnode_hashmap_del(vnode_hashmap_t* hmap, const char* key);
vnode_t* vnode_hashmap_get(vnode_hashmap_t* hmap, const char* key);
