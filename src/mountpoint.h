#pragma once

#include "mountpoint_ref.h"
#include "vnode_ref.h"

struct mountpoint
{
    vnode_ref_t root;
};

