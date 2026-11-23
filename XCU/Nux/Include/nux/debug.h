#ifndef NUX_DEBUG_H
#define NUX_DEBUG_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/thread.h>
typedef struct { nux_u64 samples; nux_u64 cpu_time_ns; nux_u64 context_switches; nux_u64 ipc_messages; nux_u64 page_faults; } nux_profile_info;
nux_status nux_debug_system_init(void);
nux_status nux_task_profile(nux_u32 task_id, nux_profile_info *out);
nux_status nux_thread_profile(nux_thread_id thread, nux_profile_info *out);
nux_status nux_debug_read_memory(nux_u32 task_id, nux_u64 address, void *buffer, nux_size length);
nux_status nux_debug_write_memory(nux_u32 task_id, nux_u64 address, const void *buffer, nux_size length);
nux_status nux_debug_get_thread_state(nux_thread_id thread, nux_machine_state *out);
nux_status nux_debug_set_thread_state(nux_thread_id thread, const nux_machine_state *state);
void nux_kernel_debugger_enter(nux_u32 reason);
#endif
