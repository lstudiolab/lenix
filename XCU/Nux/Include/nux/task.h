#ifndef NUX_TASK_H
#define NUX_TASK_H

#include <nux/types.h>
#include <nux/status.h>
#include <nux/vm.h>

#define NUX_TASK_LIMIT 128u
#define NUX_TASK_NAME_LENGTH 32u

typedef nux_u32 nux_task_id;
typedef enum nux_task_state {
    NUX_TASK_UNUSED = 0,
    NUX_TASK_READY,
    NUX_TASK_RUNNING,
    NUX_TASK_BLOCKED,
    NUX_TASK_DYING
} nux_task_state;

typedef struct nux_task_info {
    nux_task_id id;
    nux_task_state state;
    nux_u8 priority;
    nux_u8 reserved[3];
    nux_u64 quantum;
    nux_u64 runtime;
    nux_u64 address_space;
    char name[NUX_TASK_NAME_LENGTH];
} nux_task_info;

nux_status nux_task_system_init(void);
nux_status nux_task_create(const char *name, nux_u8 priority, nux_task_id *id);
nux_status nux_task_set_state(nux_task_id id, nux_task_state state);
nux_status nux_task_get(nux_task_id id, nux_task_info *out);
nux_status nux_task_get_address_space(nux_task_id id, nux_u64 *space_id);
nux_task_id nux_scheduler_choose(void);
void nux_scheduler_tick(void);

#endif
