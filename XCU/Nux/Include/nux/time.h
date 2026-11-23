#ifndef NUX_TIME_H
#define NUX_TIME_H
#include <nux/types.h>
#include <nux/status.h>
typedef nux_u64 nux_clock_id;
typedef nux_u64 nux_timer_id;
enum { NUX_CLOCK_MONOTONIC=1, NUX_CLOCK_REALTIME=2, NUX_CLOCK_BOOT=3, NUX_CLOCK_CPU=4 };
typedef struct { nux_u64 seconds; nux_u32 nanoseconds; nux_u32 reserved; } nux_timespec;
typedef struct { nux_u64 deadline_ns, interval_ns, expirations; nux_u8 armed, active; } nux_timer_info;
nux_status nux_time_system_init(void);
nux_u64 nux_clock_now(nux_clock_id clock);
nux_status nux_clock_get_resolution(nux_clock_id clock, nux_u64 *nanoseconds);
nux_status nux_timer_create(nux_clock_id clock, nux_timer_id *out);
nux_status nux_timer_arm(nux_timer_id timer, nux_u64 deadline_ns, nux_u64 interval_ns);
nux_status nux_timer_cancel(nux_timer_id timer);
nux_status nux_timer_get_info(nux_timer_id timer, nux_timer_info *out);
nux_u32 nux_timer_poll(void);
nux_status nux_thread_sleep_until(nux_u64 deadline_ns);
nux_status nux_thread_sleep_for(nux_u64 duration_ns);
nux_status nux_time_account_thread(nux_u32 thread_id, nux_u64 elapsed_ns);
nux_u64 nux_time_thread_runtime(nux_u32 thread_id);
#endif
