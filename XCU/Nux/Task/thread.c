#include <nux/thread.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

typedef struct {
    nux_thread_info info;
    nux_machine_state machine;
    nux_u8 machine_state_valid;
} nux_thread_entry;

static nux_thread_entry threads[NUX_THREAD_LIMIT];
static volatile nux_u32 thread_lock;
static nux_thread_id next_thread_id;

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
        threads[i].machine_state_valid = 0;
    }
    next_thread_id = 1;
    nux_arch_unlock(&thread_lock);
    return NUX_OK;
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
        entry->machine_state_valid = 0;
        *out = id;
        nux_arch_unlock(&thread_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&thread_lock);
    return NUX_E_LIMIT;
}

static nux_thread_entry *nux_thread_find(nux_thread_id id)
{
    for (nux_u32 i = 0; i < NUX_THREAD_LIMIT; ++i)
        if (threads[i].info.id == id) return &threads[i];
    return NUX_NULL;
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

nux_status nux_thread_yield(void)
{
    /* A yield is unsafe until the architecture dispatcher can save and
       restore real contexts. Never pretend that metadata selection is a
       context switch. */
    return NUX_E_NOT_SUPPORTED;
}
