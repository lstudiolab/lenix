#include <nux/task.h>
#include <nux/thread.h>

static volatile nux_u32 first_count;
static volatile nux_u32 second_count;
static volatile nux_u32 failed;
static nux_u64 active_space;
static nux_u64 last_task_space;

static nux_u64 test_next_space = 1;
nux_status nux_vm_space_create(nux_u64 *out)
{
    if (!out) return NUX_E_INVALID;
    *out = test_next_space++;
    return NUX_OK;
}
nux_status nux_vm_space_activate(nux_u64 id)
{
    active_space = id;
    if (id) last_task_space = id;
    return NUX_OK;
}
nux_status nux_vm_space_destroy(nux_u64 id)
{
    return id ? NUX_OK : NUX_E_INVALID;
}

void nux_arch_lock(volatile nux_u32 *lock)
{
    while (*lock) { }
    *lock = 1;
}
void nux_arch_unlock(volatile nux_u32 *lock) { *lock = 0; }

static void first_thread(void *argument)
{
    (void)argument;
    ++first_count;
    if (nux_thread_yield() != NUX_OK) failed = 1;
    ++first_count;
}

static void second_thread(void *argument)
{
    (void)argument;
    ++second_count;
    if (nux_thread_yield() != NUX_OK) failed = 1;
    ++second_count;
}

int main(void)
{
    nux_task_id task;
    nux_thread_id first, second;
    nux_thread_info info;
    nux_u64 first_space, second_space, shared_space;

    if (nux_task_system_init() != NUX_OK) return 1;
    if (nux_thread_system_init() != NUX_OK) return 2;
    if (nux_task_create("context-test", 10, &task) != NUX_OK) return 3;
    if (nux_task_get_address_space(task, &first_space) != NUX_OK || !first_space) return 10;
    if (nux_task_create("context-test-2", 10, &task) != NUX_OK) return 11;
    if (nux_task_get_address_space(task, &second_space) != NUX_OK || second_space == first_space) return 12;
    if (nux_task_create("context-test", 10, &task) != NUX_OK) return 13;
    if (nux_task_get_address_space(task, &shared_space) != NUX_OK) return 14;
    if (nux_thread_create_kernel(task, first_thread, NUX_NULL, 10, &first) != NUX_OK) return 4;
    if (nux_task_destroy(task) != NUX_E_BUSY) return 16;
    if (nux_task_get_address_space(task, &shared_space) != NUX_OK) return 17;
    if (nux_thread_create_kernel(task, second_thread, NUX_NULL, 10, &second) != NUX_OK) return 5;
    if (nux_thread_yield() != NUX_OK) return 6;
    if (failed || first_count != 2 || second_count != 2) return 7;
    if (last_task_space != shared_space || active_space != 0) return 15;
    if (nux_thread_get_info(first, &info) != NUX_OK ||
        info.state != NUX_THREAD_TERMINATED) return 8;
    if (nux_thread_get_info(second, &info) != NUX_OK ||
        info.state != NUX_THREAD_TERMINATED) return 9;
    if (nux_task_destroy(task) != NUX_OK) return 18;
    if (nux_task_get_address_space(task, &shared_space) != NUX_E_NOT_FOUND) return 19;
    return 0;
}
