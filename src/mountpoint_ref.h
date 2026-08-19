#pragma once

typedef struct mountpoint mountpoint_t;

typedef struct mountpoint_ref 
{
    mountpoint_t* _Atomic ptr;
} mountpoint_ref_t;

typedef struct __attribute__((packed))
{
    mountpoint_t* _Atomic ptr;
} mountpoint_ref_struct_t;

// Macros to dereference atomically
#define mountpoint_ref_dereference(ref) ((mountpoint_ref_t){ (ref).ptr })
#define mountpoint_struct_dereference(ref) ((mountpoint_ref_struct_t){ (ref).ptr })
