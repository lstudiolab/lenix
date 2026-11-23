#include <nux/task.h>
#include <nux/thread.h>
#include <nux/boot.h>
#include <nux/time.h>
#include <nux/processor.h>

static volatile nux_u32 first_count;
static volatile nux_u32 second_count;
static volatile nux_u32 failed;
static nux_thread_id first_thread_id;
static nux_thread_id waiting_thread_id;
static nux_u64 active_space;
static nux_u64 last_task_space;

static nux_u64 test_next_space = 1;
static nux_u64 mock_clock;
void nux_boot_mark(nux_boot_phase phase){(void)phase;}
static nux_u64 mock_user_entry, mock_user_stack;
nux_status nux_vm_fault(nux_u64 space,nux_u64 address,nux_u32 access)
{ (void)space; if(address==mock_user_entry&&access==NUX_VM_PROT_EXEC)return NUX_OK; if(address+sizeof(nux_u64)==mock_user_stack&&access==NUX_VM_PROT_WRITE)return NUX_OK; return NUX_E_ACCESS; }
void nux_arch_set_kernel_stack(nux_u64 top){(void)top;}
__attribute__((noreturn)) void nux_arch_user_enter(nux_u64 entry,nux_u64 stack,nux_u64 argument,nux_u64 kernel_stack)
{(void)entry;(void)stack;(void)argument;(void)kernel_stack;for(;;){} }
nux_cpu_id nux_processor_current_id(void) { return 0; }
nux_u32 nux_processor_set_cpu_allowed(nux_processor_set_id set, nux_cpu_id cpu)
{ (void)set; (void)cpu; return 1; }
nux_u32 nux_processor_set_exists(nux_processor_set_id set) { return set == 0; }
nux_u64 nux_clock_now(nux_clock_id clock)
{
    (void)clock;
    return ++mock_clock;
}
nux_u32 nux_timer_poll(void) { return 0; }
nux_status nux_time_account_thread(nux_u32 id, nux_u64 elapsed)
{
    (void)id;
    (void)elapsed;
    return NUX_OK;
}
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
nux_status nux_port_task_destroy(nux_task_id owner)
{
    (void)owner;
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
    if (nux_thread_prepare_wait_current(&waiting_thread_id) != NUX_OK ||
        waiting_thread_id != first_thread_id)
        failed = 1;
    if (nux_thread_yield() != NUX_OK) failed = 1;
    ++first_count;
}

static void second_thread(void *argument)
{
    (void)argument;
    ++second_count;
    if (nux_thread_wake(waiting_thread_id) != NUX_OK) failed = 1;
    if (nux_thread_yield() != NUX_OK) failed = 1;
    ++second_count;
}

int main(void)
{
    nux_task_id task;
    nux_thread_id first, second;
    nux_thread_info info;
    nux_u64 first_space, second_space, shared_space;
    nux_thread_id user_thread;
    mock_user_entry=0x400000;mock_user_stack=0x7ffefffffb00ULL;

    if (nux_task_system_init() != NUX_OK) return 1;
    if (nux_thread_system_init() != NUX_OK) return 2;
    if (nux_task_create("context-test", 10, &task) != NUX_OK) return 3;
    if (nux_task_get_address_space(task, &first_space) != NUX_OK || !first_space) return 10;
    if (nux_task_create("context-test-2", 10, &task) != NUX_OK) return 11;
    if (nux_task_get_address_space(task, &second_space) != NUX_OK || second_space == first_space) return 12;
    if (nux_task_create("context-test", 10, &task) != NUX_OK) return 13;
    if (nux_task_get_address_space(task, &shared_space) != NUX_OK) return 14;
    if (nux_thread_create_user(task,mock_user_entry,mock_user_stack,123,10,&user_thread)!=NUX_OK)return 20;
    if(nux_thread_get_info(user_thread,&info)!=NUX_OK||!info.user_mode||info.state!=NUX_THREAD_READY)return 21;
    if(nux_thread_terminate(user_thread)!=NUX_OK)return 22;
    if (nux_thread_create_kernel(task, first_thread, NUX_NULL, 10, &first) != NUX_OK) return 4;
    first_thread_id = first;
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
