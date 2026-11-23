#include <nux/task.h>

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
    if (name == NUX_NULL) return 0;
    while (name[length] != '\0' && length + 1 < NUX_TASK_NAME_LENGTH)
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
    if (name == NUX_NULL || id == NUX_NULL || priority > 31)
        return NUX_E_INVALID;
    length = nux_name_length(name);
    if (length == 0) return NUX_E_INVALID;

    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].state != NUX_TASK_UNUSED) continue;
        nux_task_id assigned = next_id++;
        if (assigned == 0) assigned = next_id++;
        tasks[i].id = assigned;
        tasks[i].state = NUX_TASK_READY;
        tasks[i].priority = priority;
        tasks[i].quantum = 5;
        tasks[i].runtime = 0;
        for (nux_size j = 0; j < length; ++j) tasks[i].name[j] = name[j];
        tasks[i].name[length] = '\0';
        *id = assigned;
        nux_arch_unlock(&task_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&task_lock);
    return NUX_E_LIMIT;
}

nux_status nux_task_set_state(nux_task_id id, nux_task_state state)
{
    if (id == 0 || state < NUX_TASK_READY || state > NUX_TASK_DYING)
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

nux_status nux_task_get(nux_task_id id, nux_task_info *out)
{
    if (id == 0 || out == NUX_NULL) return NUX_E_INVALID;
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

nux_task_id nux_scheduler_choose(void)
{
    nux_task_id chosen = 0;
    nux_arch_lock(&task_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (tasks[i].state == NUX_TASK_RUNNING)
            tasks[i].state = NUX_TASK_READY;
    }
    current_id = 0;
    for (nux_u32 n = 0; n < NUX_TASK_LIMIT; ++n) {
        nux_u32 i = (cursor + n) % NUX_TASK_LIMIT;
        if (tasks[i].state != NUX_TASK_READY) continue;
        tasks[i].state = NUX_TASK_RUNNING;
        tasks[i].runtime++;
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
