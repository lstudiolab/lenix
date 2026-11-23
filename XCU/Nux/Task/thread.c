#include <nux/thread.h>
#include <nux/time.h>
#include <nux/vm.h>
#include <nux/processor.h>
#include <nux/exception.h>
#include <nux/user.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);
extern void nux_arch_context_switch(nux_arch_context *old_context,
                                    const nux_arch_context *new_context);

typedef struct {
    nux_thread_info info;
    nux_machine_state machine;
    nux_arch_context context;
    nux_thread_entry_fn entry;
    void *argument;
    nux_u8 machine_state_valid;
    nux_u8 kernel_thread;
    nux_u8 user_thread;
    nux_u8 reserved[5];
    nux_u64 user_entry;
    nux_u64 user_stack;
    nux_u64 user_argument;
    nux_u8 stack[NUX_KERNEL_THREAD_STACK_SIZE] __attribute__((aligned(16)));
} nux_thread_entry;

static nux_thread_entry threads[NUX_THREAD_LIMIT];
static nux_arch_context idle_context;
static volatile nux_u32 thread_lock;
static nux_thread_id next_thread_id;
static nux_thread_id current_thread_id;
static nux_u32 scheduler_cursor;
static nux_u64 thread_slice_start_ns;

static nux_thread_entry *nux_thread_find(nux_thread_id id)
{
    for (nux_u32 i = 0; i < NUX_THREAD_LIMIT; ++i)
        if (threads[i].info.id == id) return &threads[i];
    return NUX_NULL;
}

static void nux_thread_bootstrap(nux_thread_id id)
{
    nux_thread_entry_fn entry = NUX_NULL;
    void *argument = NUX_NULL;

    nux_arch_lock(&thread_lock);
    nux_thread_entry *thread = nux_thread_find(id);
    if (thread && thread->kernel_thread) {
        entry = thread->entry;
        argument = thread->argument;
    }
    nux_arch_unlock(&thread_lock);

    if (entry) entry(argument);
    (void)nux_thread_terminate(id);
    (void)nux_thread_yield();

    for (;;) {
#if defined(__aarch64__)
        __asm__ volatile("msr daifset, #0xf; wfe" ::: "memory");
#else
        __asm__ volatile("cli; hlt" ::: "memory");
#endif
    }
}

nux_status nux_thread_system_init(void)
{
    nux_arch_lock(&thread_lock);
    for (nux_u32 i = 0; i < NUX_THREAD_LIMIT; ++i) {
        threads[i].info.id = 0;
        threads[i].info.task = 0;
        threads[i].info.state = NUX_THREAD_NEW;
        threads[i].info.policy = NUX_SCHED_TIMESHARE;
        threads[i].info.priority = 0;
        threads[i].info.quantum_ns = 0;
        threads[i].info.runtime_ns = 0;
        threads[i].info.deadline_ns = 0;
        threads[i].info.period_ns = 0;
        threads[i].info.budget_ns = 0;
        threads[i].info.processor_set = 0;
        threads[i].info.user_mode = 0;
        threads[i].entry = NUX_NULL;
        threads[i].argument = NUX_NULL;
        threads[i].machine_state_valid = 0;
        threads[i].kernel_thread = 0;
        threads[i].user_thread = 0;
        threads[i].user_entry = 0;
        threads[i].user_stack = 0;
        threads[i].user_argument = 0;
    }
    next_thread_id = 1;
    current_thread_id = 0;
    scheduler_cursor = 0;
    thread_slice_start_ns = 0;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_u32 nux_thread_task_busy(nux_task_id task)
{
    nux_u32 busy = 0;
    if (!task) return 0;
    nux_arch_lock(&thread_lock);
    for (nux_u32 i = 0; i < NUX_THREAD_LIMIT; ++i) {
        if (threads[i].info.id && threads[i].info.task == task &&
            threads[i].info.state != NUX_THREAD_TERMINATED) {
            busy = 1;
            break;
        }
    }
    nux_arch_unlock(&thread_lock);
    return busy;
}

nux_status nux_thread_create(nux_task_id task, nux_u8 priority, nux_thread_id *out)
{
    nux_task_info task_info;
    if (task == 0 || out == NUX_NULL || priority > 127)
        return NUX_E_INVALID;
    nux_status status = nux_task_get(task, &task_info);
    if (status != NUX_OK) return status;
    if (task_info.state == NUX_TASK_DYING || task_info.state == NUX_TASK_UNUSED)
        return NUX_E_BAD_STATE;

    nux_arch_lock(&thread_lock);
    for (nux_u32 i = 0; i < NUX_THREAD_LIMIT; ++i) {
        nux_thread_entry *entry = &threads[i];
        if (entry->info.id != 0 &&
            entry->info.state != NUX_THREAD_TERMINATED)
            continue;
        nux_thread_id id = next_thread_id++;
        if (id == 0) id = next_thread_id++;
        entry->info.id = id;
        entry->info.task = task;
        entry->info.state = NUX_THREAD_READY;
        entry->info.policy = NUX_SCHED_TIMESHARE;
        entry->info.priority = priority;
        entry->info.quantum_ns = 5000000ULL;
        entry->info.runtime_ns = 0;
        entry->info.deadline_ns = 0;
        entry->info.period_ns = 0;
        entry->info.budget_ns = 0;
        entry->info.processor_set = 0;
        entry->info.user_mode = 0;
        entry->entry = NUX_NULL;
        entry->argument = NUX_NULL;
        entry->machine_state_valid = 0;
        entry->kernel_thread = 0;
        entry->user_thread = 0;
        entry->user_entry = 0;
        entry->user_stack = 0;
        entry->user_argument = 0;
        *out = id;
        nux_arch_unlock(&thread_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&thread_lock);
    return NUX_E_LIMIT;
}

nux_status nux_thread_create_kernel(nux_task_id task, nux_thread_entry_fn entry,
                                    void *argument, nux_u8 priority,
                                    nux_thread_id *out)
{
    nux_thread_id id;
    if (!entry || !out) return NUX_E_INVALID;
    nux_status status = nux_thread_create(task, priority, &id);
    if (status != NUX_OK) return status;

    nux_arch_lock(&thread_lock);
    nux_thread_entry *thread = nux_thread_find(id);
    if (!thread) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_BAD_STATE;
    }
    thread->entry = entry;
    thread->argument = argument;
    thread->kernel_thread = 1;
    nux_uptr top = (nux_uptr)(thread->stack + sizeof(thread->stack));
#if defined(__x86_64__)
    top &= ~(nux_uptr)0xf;
    top -= sizeof(nux_u64);
    *((nux_u64 *)top) = 0;
    thread->context.rbx = 0;
    thread->context.rbp = 0;
    thread->context.r12 = 0;
    thread->context.r13 = 0;
    thread->context.r14 = 0;
    thread->context.r15 = 0;
    thread->context.rsp = top;
    thread->context.rip = (nux_u64)(nux_uptr)nux_thread_bootstrap;
    thread->context.rdi = id;
#elif defined(__aarch64__)
    top &= ~(nux_uptr)0xf;
    thread->context.x19 = 0;
    thread->context.x20 = 0;
    thread->context.x21 = 0;
    thread->context.x22 = 0;
    thread->context.x23 = 0;
    thread->context.x24 = 0;
    thread->context.x25 = 0;
    thread->context.x26 = 0;
    thread->context.x27 = 0;
    thread->context.x28 = 0;
    thread->context.x29 = 0;
    thread->context.x30 = 0;
    thread->context.sp = top;
    thread->context.pc = (nux_u64)(nux_uptr)nux_thread_bootstrap;
    thread->context.x0 = id;
#endif
    *out = id;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_status nux_thread_terminate(nux_thread_id id)
{
    if (id == 0) return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *entry = nux_thread_find(id);
    if (entry == NUX_NULL || entry->info.state == NUX_THREAD_TERMINATED) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->info.state == NUX_THREAD_RUNNING && id != current_thread_id) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_BUSY;
    }
    entry->info.state = NUX_THREAD_TERMINATED;
    entry->machine_state_valid = 0;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_status nux_thread_suspend(nux_thread_id id)
{
    if (id == 0) return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *entry = nux_thread_find(id);
    if (entry == NUX_NULL || entry->info.state == NUX_THREAD_TERMINATED) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->info.state == NUX_THREAD_RUNNING) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_BUSY;
    }
    entry->info.state = NUX_THREAD_SUSPENDED;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_status nux_thread_resume(nux_thread_id id)
{
    if (id == 0) return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *entry = nux_thread_find(id);
    if (entry == NUX_NULL || entry->info.state == NUX_THREAD_TERMINATED) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->info.state != NUX_THREAD_SUSPENDED) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_BAD_STATE;
    }
    entry->info.state = NUX_THREAD_READY;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_status nux_thread_get_info(nux_thread_id id, nux_thread_info *out)
{
    if (id == 0 || out == NUX_NULL) return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *entry = nux_thread_find(id);
    if (entry == NUX_NULL) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    *out = entry->info;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_status nux_thread_get_state(nux_thread_id id, nux_machine_state *out)
{
    if (id == 0 || out == NUX_NULL) return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *entry = nux_thread_find(id);
    if (entry == NUX_NULL || !entry->machine_state_valid) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    *out = entry->machine;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_status nux_thread_set_state(nux_thread_id id, const nux_machine_state *state)
{
    if (id == 0 || state == NUX_NULL) return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *entry = nux_thread_find(id);
    if (entry == NUX_NULL || entry->info.state == NUX_THREAD_TERMINATED) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->info.state == NUX_THREAD_RUNNING) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_BUSY;
    }
    entry->machine = *state;
    entry->machine_state_valid = 1;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_status nux_thread_set_policy(nux_thread_id id, nux_sched_policy policy,
                                 nux_u8 priority, nux_u64 quantum_ns)
{
    if (id == 0 || policy > NUX_SCHED_REALTIME || priority > 127 ||
        (policy != NUX_SCHED_REALTIME && quantum_ns == 0))
        return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *entry = nux_thread_find(id);
    if (entry == NUX_NULL || entry->info.state == NUX_THREAD_TERMINATED) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    entry->info.policy = policy;
    entry->info.priority = priority;
    entry->info.quantum_ns = quantum_ns;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_status nux_thread_set_time_constraint(nux_thread_id id, nux_u64 period_ns,
                                          nux_u64 budget_ns, nux_u64 deadline_ns)
{
    if (id == 0 || period_ns == 0 || budget_ns == 0 ||
        budget_ns > period_ns || deadline_ns == 0)
        return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *entry = nux_thread_find(id);
    if (entry == NUX_NULL || entry->info.state == NUX_THREAD_TERMINATED) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    entry->info.policy = NUX_SCHED_REALTIME;
    entry->info.period_ns = period_ns;
    entry->info.budget_ns = budget_ns;
    entry->info.deadline_ns = deadline_ns;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}


static nux_u32 nux_thread_task_runnable(nux_task_id task_id)
{
    nux_task_info task;
    if (nux_task_get(task_id, &task) != NUX_OK) return 0;
    return task.state != NUX_TASK_SUSPENDED &&
        task.state != NUX_TASK_BLOCKED &&
        task.state != NUX_TASK_DYING &&
        task.state != NUX_TASK_UNUSED;
}

nux_status nux_thread_yield(void)
{
    nux_thread_entry *old_thread;
    nux_thread_entry *next_thread = NUX_NULL;
    nux_arch_context *old_context;
    nux_thread_id old_id;
    nux_thread_state old_state = NUX_THREAD_NEW;

    (void)nux_timer_poll();
    nux_arch_lock(&thread_lock);
    old_id = current_thread_id;
    old_thread = old_id ? nux_thread_find(old_id) : NUX_NULL;
    if (old_thread) old_state = old_thread->info.state;
    nux_u64 now_ns = nux_clock_now(NUX_CLOCK_MONOTONIC);
    if (old_thread && thread_slice_start_ns && now_ns >= thread_slice_start_ns) {
        nux_u64 elapsed = now_ns - thread_slice_start_ns;
        if (elapsed > NUX_U64_MAX - old_thread->info.runtime_ns)
            old_thread->info.runtime_ns = NUX_U64_MAX;
        else
            old_thread->info.runtime_ns += elapsed;
        (void)nux_time_account_thread(old_id, elapsed);
    }
    if (old_thread && old_thread->info.state == NUX_THREAD_RUNNING)
        old_thread->info.state = NUX_THREAD_READY;

    for (nux_u32 n = 0; n < NUX_THREAD_LIMIT; ++n) {
        nux_u32 i = (scheduler_cursor + n) % NUX_THREAD_LIMIT;
        nux_thread_entry *candidate = &threads[i];
        if (!candidate->kernel_thread ||
            candidate->info.state != NUX_THREAD_READY ||
            !nux_thread_task_runnable(candidate->info.task) ||
            !nux_processor_set_cpu_allowed(candidate->info.processor_set,
                                           nux_processor_current_id()))
            continue;
        next_thread = candidate;
        scheduler_cursor = (i + 1) % NUX_THREAD_LIMIT;
        break;
    }

    if (next_thread) {
        nux_u64 address_space = 0;
        nux_status vm_status = nux_task_get_address_space(
            next_thread->info.task, &address_space);
        if (vm_status == NUX_OK)
            vm_status = nux_vm_space_activate(address_space);
        if (vm_status != NUX_OK) {
            if (old_thread) old_thread->info.state = old_state;
            nux_arch_unlock(&thread_lock);
            return vm_status;
        }
        next_thread->info.state = NUX_THREAD_RUNNING;
        current_thread_id = next_thread->info.id;
        thread_slice_start_ns = now_ns;
        if (next_thread == old_thread) {
            nux_arch_unlock(&thread_lock);
            return NUX_OK;
        }
        old_context = old_thread ? &old_thread->context : &idle_context;
        nux_arch_unlock(&thread_lock);
        nux_arch_context_switch(old_context, &next_thread->context);
        return NUX_OK;
    }

    if (old_thread && old_thread->info.state == NUX_THREAD_READY &&
        nux_thread_task_runnable(old_thread->info.task) &&
        nux_processor_set_cpu_allowed(old_thread->info.processor_set,
                                      nux_processor_current_id())) {
        old_thread->info.state = NUX_THREAD_RUNNING;
        current_thread_id = old_thread->info.id;
        thread_slice_start_ns = now_ns;
        nux_arch_unlock(&thread_lock);
        return NUX_OK;
    }

    if (old_thread) {
        (void)nux_vm_space_activate(0);
        current_thread_id = 0;
        thread_slice_start_ns = 0;
        nux_arch_unlock(&thread_lock);
        nux_arch_context_switch(&old_thread->context, &idle_context);
        return NUX_OK;
    }

    nux_arch_unlock(&thread_lock);
    return NUX_E_WOULD_BLOCK;
}

nux_status nux_thread_set_processor_set(nux_thread_id id, nux_u32 set)
{
    if (!id || (set && !nux_processor_set_exists(set))) return NUX_E_INVALID;
    nux_arch_lock(&thread_lock);
    if (set && !nux_processor_set_exists(set)) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    nux_thread_entry *entry = nux_thread_find(id);
    if (!entry || entry->info.state == NUX_THREAD_TERMINATED) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->info.state == NUX_THREAD_RUNNING) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_BUSY;
    }
    entry->info.processor_set = set;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}

nux_u32 nux_thread_processor_set_busy(nux_u32 set)
{
    if (!set) return 0;
    nux_u32 busy = 0;
    nux_arch_lock(&thread_lock);
    for (nux_u32 i = 0; i < NUX_THREAD_LIMIT; ++i) {
        if (threads[i].info.id && threads[i].info.state != NUX_THREAD_TERMINATED &&
            threads[i].info.processor_set == set) {
            busy = 1;
            break;
        }
    }
    nux_arch_unlock(&thread_lock);
    return busy;
}

static void nux_thread_user_entry(void *argument)
{
    nux_thread_id id = (nux_thread_id)(nux_uptr)argument;
    nux_u64 entry = 0, stack = 0, user_argument = 0, kernel_stack_top = 0;
    nux_arch_lock(&thread_lock);
    nux_thread_entry *thread = nux_thread_find(id);
    if (thread && thread->user_thread) {
        entry = thread->user_entry;
        stack = thread->user_stack;
        user_argument = thread->user_argument;
        kernel_stack_top = (nux_u64)(nux_uptr)
            (thread->stack + sizeof(thread->stack));
    }
    nux_arch_unlock(&thread_lock);
    if (!entry || !stack || !kernel_stack_top) return;
    nux_arch_set_kernel_stack(kernel_stack_top);
    nux_arch_user_enter(entry, stack, user_argument);
}

nux_status nux_thread_create_user(nux_task_id task, nux_u64 entry,
                                  nux_u64 stack, nux_u64 argument,
                                  nux_u8 priority, nux_thread_id *out)
{
    nux_u64 address_space;
    if (!task || !entry || !stack || !out || (stack & 0xfULL) ||
        priority > 127)
        return NUX_E_INVALID;
    if (nux_task_get_address_space(task, &address_space) != NUX_OK ||
        !address_space)
        return NUX_E_BAD_STATE;
    if (nux_vm_fault(address_space, entry, NUX_VM_PROT_EXEC) != NUX_OK ||
        nux_vm_fault(address_space, stack, NUX_VM_PROT_WRITE) != NUX_OK)
        return NUX_E_ACCESS;

    nux_thread_id id;
    nux_status status = nux_thread_create_kernel(
        task, nux_thread_user_entry, (void *)(nux_uptr)0, priority, &id);
    if (status != NUX_OK) return status;

    nux_arch_lock(&thread_lock);
    nux_thread_entry *thread = nux_thread_find(id);
    if (!thread) {
        nux_arch_unlock(&thread_lock);
        return NUX_E_BAD_STATE;
    }
    thread->user_thread = 1;
    thread->info.user_mode = 1;
    thread->user_entry = entry;
    thread->user_stack = stack;
    thread->user_argument = argument;
    thread->argument = (void *)(nux_uptr)id;
    *out = id;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
}
