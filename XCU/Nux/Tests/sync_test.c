#include <nux/sync.h>

void nux_arch_lock(volatile nux_u32 *lock)
{
    while (*lock) { }
    *lock = 1;
}
void nux_arch_unlock(volatile nux_u32 *lock) { *lock = 0; }

int main(void)
{
    nux_semaphore_id semaphore;
    nux_lock_set_id locks;
    nux_condition_id condition;

    if (nux_sync_system_init() != NUX_OK) return 1;
    if (nux_semaphore_create(1, 2, &semaphore) != NUX_OK) return 2;
    if (nux_semaphore_wait(semaphore, 0) != NUX_OK) return 3;
    if (nux_semaphore_wait(semaphore, 0) != NUX_E_WOULD_BLOCK) return 4;
    if (nux_semaphore_post(semaphore) != NUX_OK) return 5;
    if (nux_semaphore_post(semaphore) != NUX_OK) return 6;
    if (nux_semaphore_post(semaphore) != NUX_E_LIMIT) return 7;
    if (nux_semaphore_destroy(semaphore) != NUX_OK) return 8;

    if (nux_lock_set_create(2, &locks) != NUX_OK) return 9;
    if (nux_lock_acquire(locks, 1, 0) != NUX_OK) return 10;
    if (nux_lock_acquire(locks, 1, 0) != NUX_E_WOULD_BLOCK) return 11;
    if (nux_lock_set_destroy(locks) != NUX_E_BUSY) return 12;
    if (nux_lock_release(locks, 1) != NUX_OK) return 13;
    if (nux_lock_release(locks, 1) != NUX_E_BAD_STATE) return 14;
    if (nux_lock_set_destroy(locks) != NUX_OK) return 15;

    if (nux_condition_create(&condition) != NUX_OK) return 16;
    if (nux_condition_signal(condition) != NUX_OK) return 17;
    if (nux_condition_wait(condition, 0, 0) != NUX_E_NOT_SUPPORTED) return 18;
    if (nux_condition_destroy(condition) != NUX_OK) return 19;
    return 0;
}
