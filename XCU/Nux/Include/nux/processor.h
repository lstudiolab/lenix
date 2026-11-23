#ifndef NUX_PROCESSOR_H
#define NUX_PROCESSOR_H
#include <nux/types.h>
#include <nux/status.h>
#define NUX_CPU_LIMIT 256u
#define NUX_PROCESSOR_SET_LIMIT 64u
typedef nux_u32 nux_cpu_id;
typedef nux_u32 nux_processor_set_id;
typedef struct { nux_cpu_id id; nux_u32 architecture; nux_u32 logical_id; nux_u32 numa_node; nux_u64 features; nux_u64 frequency_hz; nux_u8 online; nux_u8 reserved[7]; } nux_processor_info;
nux_status nux_processor_system_init(void);
nux_u32 nux_processor_count(void);
nux_status nux_processor_get_info(nux_cpu_id cpu, nux_processor_info *out);
nux_status nux_processor_start(nux_cpu_id cpu);
nux_status nux_processor_stop(nux_cpu_id cpu);
nux_status nux_processor_set_create(nux_processor_set_id *out);
nux_status nux_processor_set_destroy(nux_processor_set_id set);
nux_status nux_processor_set_assign_cpu(nux_processor_set_id set, nux_cpu_id cpu);
nux_status nux_processor_set_assign_thread(nux_processor_set_id set, nux_u32 thread_id);
nux_status nux_processor_set_policy(nux_processor_set_id set, nux_u32 policy, nux_u8 priority);
nux_status nux_thread_set_affinity(nux_u32 thread_id, nux_processor_set_id set);
#endif
