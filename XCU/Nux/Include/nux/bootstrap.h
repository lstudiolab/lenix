#ifndef NUX_BOOTSTRAP_H
#define NUX_BOOTSTRAP_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/capability.h>
#define NUX_SERVICE_NAME_MAX 64u
#define NUX_BOOTSTRAP_SERVICE_LIMIT 128u
typedef struct { char name[NUX_SERVICE_NAME_MAX]; nux_capability service_port; nux_u32 owner_task; nux_u32 flags; } nux_service_entry;
nux_status nux_bootstrap_system_init(void);
nux_status nux_bootstrap_register(const char *name, nux_capability service_port);
nux_status nux_bootstrap_lookup(const char *name, nux_capability *service_port);
nux_status nux_bootstrap_unregister(const char *name);
void nux_bootstrap_task_destroy(nux_u32 owner);
nux_status nux_bootstrap_start_initial_task(nux_u64 image_address, nux_u64 image_size, nux_u32 *task_id);
nux_status nux_task_get_special_port(nux_u32 task_id, nux_u32 which, nux_capability *out);
nux_status nux_task_set_special_port(nux_u32 task_id, nux_u32 which, nux_capability port);
#endif
