#pragma once

typedef struct vnode vnode_t;

typedef struct
{
    vnode_t* _Atomic ptr;
} vnode_ref_t;

