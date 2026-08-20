#pragma once

typedef struct inode inode_t;

typedef struct __attribute__((packed))
{
    inode_t* _Atomic ptr;
} inode_ref_t;

typedef struct __attribute__((packed))
{
    inode_t* _Atomic ptr;
} inode_ref_struct_t;

// Macros to dereference atomically
#define inode_ref_dereference(ref) ((inode_ref_t){ (ref).ptr })
#define inode_struct_dereference(ref) ((inode_ref_struct_t){ (ref).ptr })

