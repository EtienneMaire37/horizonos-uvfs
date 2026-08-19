#pragma once

typedef struct vnode vnode_t;

typedef struct __attribute__((packed))
{
    vnode_t* _Atomic ptr;
} vnode_ref_t;

typedef struct __attribute__((packed))
{
    vnode_t* _Atomic ptr;
} vnode_ref_struct_t;

// Macros to dereference atomically
#define vnode_ref_dereference(ref) ((vnode_ref_t){ (ref).ptr })
#define vnode_struct_dereference(ref) ((vnode_ref_struct_t){ (ref).ptr })
