#ifndef NUX_HOST_H
#define NUX_HOST_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/capability.h>
typedef struct { nux_u32 architecture; nux_u32 cpu_count; nux_u64 physical_memory_bytes; nux_u64 boot_time_ns; nux_u64 features; } nux_host_info;
nux_status nux_host_get_info(nux_host_info *out);
nux_status nux_host_get_time(nux_u64 *monotonic_ns, nux_u64 *realtime_ns);
nux_status nux_host_reboot(nux_capability authority, nux_u32 flags);
nux_status nux_host_shutdown(nux_capability authority);
nux_status nux_host_get_special_port(nux_u32 which, nux_capability *out);
#endif
