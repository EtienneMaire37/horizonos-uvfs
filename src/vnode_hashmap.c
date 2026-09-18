#include "vnode_hashmap.h"
#include "util/hash.h"
#include "util/assert.h"
#include <stdlib.h>
#include <string.h>

vnode_hashmap_t* vnode_hashmap_create(size_t mem_limit)
{
    vnode_hashmap_t* ret = malloc(sizeof(vnode_hashmap_t));
    if (!ret) return NULL;
    ret->entries = mem_limit / sizeof(vnode_t*);
    ret->data = calloc(ret->entries, sizeof(vnode_t*));
    if (!ret->data)
    {
        free(ret);
        return NULL;
    }
    ret->lock = (atomic_flag)ATOMIC_FLAG_INIT;
    return ret;
}

void vnode_hashmap_destroy(vnode_hashmap_t** hmap)
{
    ASSERT(hmap);
    if (!*hmap) return;
    for (size_t i = 0; i < (*hmap)->entries; i++)
    {
        vnode_ll_item_t* it = (*hmap)->data[i];
        while (it)
        {
            vnode_ll_item_t* last = it;
            it = it->next;
            free(last);
        }
    }
    free((*hmap)->data);
    free(*hmap);
    *hmap = NULL;
}

vnode_t* vnode_hashmap_put(vnode_hashmap_t* hmap, const char* key, vnode_t* node)
{
    ASSERT(hmap);
    vnode_ll_item_t* it = malloc(sizeof(*it));
    if (!it) return NULL;
    uint32_t flags = acquire_spinlock_noint(&hmap->lock);
    it->node = node;
    it->key = key;
    uint64_t idx = hash_string(key) % hmap->entries;
    it->next = hmap->data[idx];
    hmap->data[idx] = it;
    release_spinlock_noint(&hmap->lock, flags);
    return node;
}

vnode_t* vnode_hashmap_set(vnode_hashmap_t* hmap, const char* key, vnode_t* node)
{
    ASSERT(hmap);
    uint32_t flags = acquire_spinlock_noint(&hmap->lock);
    uint64_t idx = hash_string(key) % hmap->entries;
    vnode_ll_item_t* val = hmap->data[idx];
    while (val)
    {
        if (strcmp(val->key, key) == 0)
        {
            val->node = node;
            release_spinlock_noint(&hmap->lock, flags);
            return node;
        }
        val = val->next;
    }
    
    vnode_ll_item_t* it = malloc(sizeof(*it));
    if (!it)
    {
        release_spinlock_noint(&hmap->lock, flags);
        return NULL;
    }
    it->node = node;
    it->key = key;
    it->next = hmap->data[idx];
    hmap->data[idx] = it;
    release_spinlock_noint(&hmap->lock, flags);
    return node;
}

vnode_t* vnode_hashmap_get(vnode_hashmap_t* hmap, const char* key)
{
    ASSERT(hmap);
    uint32_t flags = acquire_spinlock_noint(&hmap->lock);
    uint64_t idx = hash_string(key) % hmap->entries;
    vnode_ll_item_t* val = hmap->data[idx];
    while (val)
    {
        if (strcmp(val->key, key) == 0)
        {
            vnode_t* node = val->node;
            release_spinlock_noint(&hmap->lock, flags);
            return node;
        }
        val = val->next;
    }
    release_spinlock_noint(&hmap->lock, flags);
    return NULL;
}

void vnode_hashmap_del(vnode_hashmap_t* hmap, const char* key)
{
    ASSERT(hmap);
    uint32_t flags = acquire_spinlock_noint(&hmap->lock);
    uint64_t idx = hash_string(key) % hmap->entries;
    vnode_ll_item_t* val = hmap->data[idx];
    vnode_ll_item_t* prev = NULL;
    while (val)
    {
        if (strcmp(val->key, key) == 0)
        {
            if (!prev)
                hmap->data[idx] = val->next;
            else
                prev->next = val->next;
            free(val);
            release_spinlock_noint(&hmap->lock, flags);
            return;
        }
        prev = val;
        val = val->next;
    }
    release_spinlock_noint(&hmap->lock, flags);
}

