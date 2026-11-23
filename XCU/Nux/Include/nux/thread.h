#ifndef NUX_THREAD_H
#define NUX_THREAD_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/task.h>
#define NUX_THREAD_LIMIT 512u
typedef nux_u32 nux_thread_id;
typedef enum { NUX_THREAD_NEW=0, NUX_THREAD_READY, NUX_THREAD_RUNNING, NUX_THREAD_WAITING, NUX_THREAD_SUSPENDED, NUX_THREAD_TERMINATED } nux_thread_state;
typedef enum { NUX_SCHED_TIMESHARE=0, NUX_SCHED_FIXED_PRIORITY, NUX_SCHED_REALTIME } nux_sched_policy;
typedef struct { nux_thread_id id; nux_task_id task; nux_thread_state state; nux_sched_policy policy; nux_u8 priority; nux_u64 quantum_ns; nux_u64 runtime_ns; nux_u64 deadline_ns; nux_u64 period_ns; nux_u64 budget_ns; } nux_thread_info;
typedef struct { nux_u64 x[31]; nux_u64 sp; nux_u64 pc; nux_u64 flags; } nux_machine_state;
nux_status nux_thread_system_init(void);
nux_status nux_thread_create(nux_task_id task, nux_u8 priority, nux_thread_id *out);
nux_status nux_thread_terminate(nux_thread_id thread);
nux_status nux_thread_suspend(nux_thread_id thread);
nux_status nux_thread_resume(nux_thread_id thread);
nux_status nux_thread_get_info(nux_thread_id thread, nux_thread_info *out);
nux_status nux_thread_get_state(nux_thread_id thread, nux_machine_state *out);
nux_status nux_thread_set_state(nux_thread_id thread, const nux_machine_state *state);
nux_status nux_thread_set_policy(nux_thread_id thread, nux_sched_policy policy, nux_u8 priority, nux_u64 quantum_ns);
nux_status nux_thread_set_time_constraint(nux_thread_id thread, nux_u64 period_ns, nux_u64 budget_ns, nux_u64 deadline_ns);
nux_status nux_thread_yield(void);
#endif
