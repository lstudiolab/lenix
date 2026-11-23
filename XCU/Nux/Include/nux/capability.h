#ifndef NUX_CAPABILITY_H
#define NUX_CAPABILITY_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/object.h>
#define NUX_CAPABILITY_LIMIT 255u
typedef nux_u32 nux_capability;
#define NUX_OBJECT_ENDPOINT NUX_OBJECT_PORT
#define NUX_RIGHT_GRANT NUX_RIGHT_TRANSFER
#define NUX_RIGHT_ADMIN NUX_RIGHT_CONTROL
nux_status nux_capability_system_init(void);
nux_status nux_capability_create(nux_object_type type, nux_object_id object_id, nux_rights rights, nux_capability *out);
nux_status nux_capability_check(nux_capability cap, nux_object_type type, nux_rights required_rights, nux_object_id *object_id);
nux_status nux_capability_revoke(nux_capability cap);
nux_u32 nux_capability_revoke_object(nux_object_type type, nux_object_id object_id);
#endif
