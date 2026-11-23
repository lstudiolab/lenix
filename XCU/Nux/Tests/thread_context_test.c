#include <nux/task.h>
#include <nux/thread.h>

static volatile nux_u32 first_count;
static volatile nux_u32 second_count;
static volatile nux_u32 failed;

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

    if (nux_task_system_init() != NUX_OK) return 1;
    if (nux_thread_system_init() != NUX_OK) return 2;
    if (nux_task_create("context-test", 10, &task) != NUX_OK) return 3;
    if (nux_thread_create_kernel(task, first_thread, NUX_NULL, 10, &first) != NUX_OK) return 4;
    if (nux_thread_create_kernel(task, second_thread, NUX_NULL, 10, &second) != NUX_OK) return 5;
    if (nux_thread_yield() != NUX_OK) return 6;
    if (failed || first_count != 2 || second_count != 2) return 7;
    if (nux_thread_get_info(first, &info) != NUX_OK ||
        info.state != NUX_THREAD_TERMINATED) return 8;
    if (nux_thread_get_info(second, &info) != NUX_OK ||
        info.state != NUX_THREAD_TERMINATED) return 9;
    return 0;
}
