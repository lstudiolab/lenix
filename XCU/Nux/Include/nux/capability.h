#ifndef NUX_CAPABILITY_H
#define NUX_CAPABILITY_H

#include <nux/types.h>
#include <nux/status.h>

#define NUX_CAPABILITY_LIMIT 256u

typedef nux_u32 nux_capability;
typedef enum nux_object_type {
    NUX_OBJECT_NONE = 0,
    NUX_OBJECT_TASK = 1,
    NUX_OBJECT_ENDPOINT = 2,
    NUX_OBJECT_MEMORY = 3
} nux_object_type;

enum {
    NUX_RIGHT_READ  = 1u << 0,
    NUX_RIGHT_WRITE = 1u << 1,
    NUX_RIGHT_GRANT = 1u << 2,
    NUX_RIGHT_ADMIN = 1u << 3
};

nux_status nux_capability_system_init(void);
nux_status nux_capability_create(nux_object_type type, nux_u32 object_id,
                                 nux_u32 rights, nux_capability *out);
nux_status nux_capability_check(nux_capability cap, nux_object_type type,
                                nux_u32 required_rights, nux_u32 *object_id);
nux_status nux_capability_revoke(nux_capability cap);

#endif
