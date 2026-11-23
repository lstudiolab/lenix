#ifndef NUX_SYNC_H
#define NUX_SYNC_H
#include <nux/types.h>
#include <nux/status.h>
#define NUX_SYNC_OBJECT_LIMIT 512u
typedef nux_u32 nux_semaphore_id;
typedef nux_u32 nux_lock_set_id;
typedef nux_u32 nux_condition_id;
nux_status nux_sync_system_init(void);
nux_status nux_semaphore_create(nux_u32 initial_count, nux_u32 maximum_count, nux_semaphore_id *out);
nux_status nux_semaphore_wait(nux_semaphore_id semaphore, nux_u64 timeout_ns);
nux_status nux_semaphore_post(nux_semaphore_id semaphore);
nux_status nux_semaphore_post_all(nux_semaphore_id semaphore);
nux_status nux_semaphore_destroy(nux_semaphore_id semaphore);
nux_status nux_lock_set_create(nux_u32 lock_count, nux_lock_set_id *out);
nux_status nux_lock_acquire(nux_lock_set_id set, nux_u32 lock_index, nux_u64 timeout_ns);
nux_status nux_lock_release(nux_lock_set_id set, nux_u32 lock_index);
nux_status nux_lock_set_destroy(nux_lock_set_id set);
nux_status nux_condition_create(nux_condition_id *out);
nux_status nux_condition_wait(nux_condition_id condition, nux_u32 mutex_id, nux_u64 timeout_ns);
nux_status nux_condition_signal(nux_condition_id condition);
nux_status nux_condition_broadcast(nux_condition_id condition);
nux_status nux_condition_destroy(nux_condition_id condition);
#endif
