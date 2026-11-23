#ifndef NUX_OBJECT_H
#define NUX_OBJECT_H

#include <nux/types.h>

/*
 * Kernel object identifiers are opaque handles, never direct pointers
 * supplied by user tasks.
 */
typedef nux_u64 nux_object_id;
typedef nux_u32 nux_object_type;

enum nux_object_type {
    NUX_OBJECT_NONE = 0,
    NUX_OBJECT_TASK,
    NUX_OBJECT_THREAD,
    NUX_OBJECT_PORT,
    NUX_OBJECT_MEMORY
};

typedef nux_u64 nux_rights;

enum {
    NUX_RIGHT_READ    = (nux_u64)1 << 0,
    NUX_RIGHT_WRITE   = (nux_u64)1 << 1,
    NUX_RIGHT_SEND    = (nux_u64)1 << 2,
    NUX_RIGHT_RECEIVE = (nux_u64)1 << 3,
    NUX_RIGHT_MAP     = (nux_u64)1 << 4,
    NUX_RIGHT_CONTROL = (nux_u64)1 << 5,
    NUX_RIGHT_TRANSFER= (nux_u64)1 << 6
};

#endif
