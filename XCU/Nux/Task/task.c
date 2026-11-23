#include <nux/task.h>
#include <nux/thread.h>
#include <nux/port.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

static nux_task_info tasks[NUX_TASK_LIMIT];
static volatile nux_u32 task_lock;
static nux_task_id next_id;
static nux_task_id current_id;
static nux_u32 cursor;

static nux_size nux_name_length(const char *name)
{
    nux_size length = 0;
    if (!name) return 0;
    while (name[length] && length + 1 < NUX_TASK_NAME_LENGTH)
        ++length;
    return length;
}

nux_status nux_task_system_init(void)
{
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        tasks[i].id = 0;
        tasks[i].state = NUX_TASK_UNUSED;
        tasks[i].priority = 0;
        tasks[i].quantum = 0;
        tasks[i].runtime = 0;
        tasks[i].address_space = 0;
        tasks[i].name[0] = '\0';
    }
    next_id = 1;
    current_id = 0;
    cursor = 0;
    nux_arch_unlock(&task_lock);
    return NUX_OK;
}

nux_status nux_task_create(const char *name, nux_u8 priority, nux_task_id *id)
{
    nux_size length;
    nux_u64 address_space;
    nux_status status;
    if (!name || !id || priority > 31) return NUX_E_INVALID;
    length = nux_name_length(name);
    if (!length) return NUX_E_INVALID;

    status = nux_vm_space_create(&address_space);
    if (status != NUX_OK) return status;

    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].state != NUX_TASK_UNUSED) continue;
        nux_task_id assigned = next_id++;
        if (!assigned) assigned = next_id++;
        tasks[i].id = assigned;
        tasks[i].state = NUX_TASK_READY;
        tasks[i].priority = priority;
        tasks[i].quantum = 5;
        tasks[i].runtime = 0;
        tasks[i].address_space = address_space;
        for (nux_size j = 0; j < length; ++j) tasks[i].name[j] = name[j];
        tasks[i].name[length] = '\0';
        *id = assigned;
        nux_arch_unlock(&task_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&task_lock);
    (void)nux_vm_space_destroy(address_space);
    return NUX_E_LIMIT;
}

nux_status nux_task_destroy(nux_task_id id)
{
    nux_u64 address_space = 0;
    nux_task_state previous_state = NUX_TASK_UNUSED;

    if (!id) return NUX_E_INVALID;
    nux_arch_lock(&task_lock);
    nux_task_info *task = NUX_NULL;
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].id == id) {
            task = &tasks[i];
            break;
        }
    }
    if (!task || task->state == NUX_TASK_UNUSED ||
        task->state == NUX_TASK_DYING) {
        nux_arch_unlock(&task_lock);
        return task ? NUX_E_BAD_STATE : NUX_E_NOT_FOUND;
    }
    previous_state = task->state;
    task->state = NUX_TASK_DYING;
    address_space = task->address_space;
    nux_arch_unlock(&task_lock);

    /* A task cannot release its address space while one of its threads lives. */
    if (nux_thread_task_busy(id)) {
        nux_arch_lock(&task_lock);
        for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i)
            if (tasks[i].id == id && tasks[i].state == NUX_TASK_DYING)
                tasks[i].state = previous_state;
        nux_arch_unlock(&task_lock);
        return NUX_E_BUSY;
    }

    nux_status status = nux_vm_space_destroy(address_space);
    if (status != NUX_OK) {
        nux_arch_lock(&task_lock);
        for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i)
            if (tasks[i].id == id && tasks[i].state == NUX_TASK_DYING)
                tasks[i].state = previous_state;
        nux_arch_unlock(&task_lock);
        return status;
    }

    (void)nux_port_task_destroy(id);

    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].id != id) continue;
        tasks[i].id = 0;
        tasks[i].state = NUX_TASK_UNUSED;
        tasks[i].priority = 0;
        tasks[i].quantum = 0;
        tasks[i].runtime = 0;
        tasks[i].address_space = 0;
        tasks[i].name[0] = '\0';
        break;
    }
    nux_arch_unlock(&task_lock);
    return NUX_OK;
}

nux_status nux_task_set_state(nux_task_id id, nux_task_state state)
{
    if (!id || state < NUX_TASK_READY || state > NUX_TASK_DYING ||
        state == NUX_TASK_RUNNING || state == NUX_TASK_DYING)
        return NUX_E_INVALID;
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].id != id) continue;
        if (tasks[i].state == NUX_TASK_UNUSED || tasks[i].state == NUX_TASK_DYING) {
            nux_arch_unlock(&task_lock);
            return NUX_E_BAD_STATE;
        }
        tasks[i].state = state;
        nux_arch_unlock(&task_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&task_lock);
    return NUX_E_NOT_FOUND;
}


nux_status nux_task_suspend(nux_task_id id)
{
    if (!id) return NUX_E_INVALID;
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].id != id) continue;
        if (tasks[i].state == NUX_TASK_DYING ||
            tasks[i].state == NUX_TASK_UNUSED) {
            nux_arch_unlock(&task_lock);
            return NUX_E_BAD_STATE;
        }
        if (tasks[i].state == NUX_TASK_SUSPENDED) {
            nux_arch_unlock(&task_lock);
            return NUX_E_BAD_STATE;
        }
        tasks[i].state = NUX_TASK_SUSPENDED;
        nux_arch_unlock(&task_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&task_lock);
    return NUX_E_NOT_FOUND;
}

nux_status nux_task_resume(nux_task_id id)
{
    if (!id) return NUX_E_INVALID;
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].id != id) continue;
        if (tasks[i].state != NUX_TASK_SUSPENDED) {
            nux_arch_unlock(&task_lock);
            return NUX_E_BAD_STATE;
        }
        tasks[i].state = NUX_TASK_READY;
        nux_arch_unlock(&task_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&task_lock);
    return NUX_E_NOT_FOUND;
}

nux_status nux_task_get(nux_task_id id, nux_task_info *out)
{
    if (!id || !out) return NUX_E_INVALID;
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].id != id) continue;
        *out = tasks[i];
        nux_arch_unlock(&task_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&task_lock);
    return NUX_E_NOT_FOUND;
}

nux_status nux_task_get_address_space(nux_task_id id, nux_u64 *space_id)
{
    if (!id || !space_id) return NUX_E_INVALID;
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].id != id) continue;
        if (tasks[i].state == NUX_TASK_DYING) {
            nux_arch_unlock(&task_lock);
            return NUX_E_BAD_STATE;
        }
        *space_id = tasks[i].address_space;
        nux_arch_unlock(&task_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&task_lock);
    return NUX_E_NOT_FOUND;
}

nux_task_id nux_scheduler_choose(void)
{
    nux_task_id chosen = 0;
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i)
        if (tasks[i].state == NUX_TASK_RUNNING) tasks[i].state = NUX_TASK_READY;
    current_id = 0;
    for (nux_u32 n = 0; n < NUX_TASK_LIMIT; ++n) {
        nux_u32 i = (cursor + n) % NUX_TASK_LIMIT;
        if (tasks[i].state != NUX_TASK_READY) continue;
        tasks[i].state = NUX_TASK_RUNNING;
        ++tasks[i].runtime;
        current_id = tasks[i].id;
        cursor = (i + 1) % NUX_TASK_LIMIT;
        chosen = current_id;
        break;
    }
    nux_arch_unlock(&task_lock);
    return chosen;
}

void nux_scheduler_tick(void)
{
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].id == current_id && tasks[i].state == NUX_TASK_RUNNING) {
            tasks[i].state = NUX_TASK_READY;
            break;
        }
    }
    current_id = 0;
    nux_arch_unlock(&task_lock);
}
