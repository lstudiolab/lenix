#include <nux/bootstrap.h>
#include <nux/thread.h>
#include <nux/elf.h>
#include <nux/object.h>

static nux_task_id current_task = 1;
static nux_capability next_capability = 10;
static nux_object_id registered_object = 42;
static nux_u32 capability_creates;

void nux_arch_lock(volatile nux_u32 *lock) { (void)lock; }
void nux_arch_unlock(volatile nux_u32 *lock) { (void)lock; }
nux_task_id nux_thread_current_task(void) { return current_task; }

nux_status nux_capability_check(nux_u32 owner, nux_capability cap,
                                nux_object_type type, nux_rights rights,
                                nux_object_id *object_id)
{
    if (!owner || !cap || type != NUX_OBJECT_PORT ||
        !(rights & NUX_RIGHT_SEND) || !object_id)
        return NUX_E_ACCESS;
    *object_id = registered_object;
    return NUX_OK;
}

nux_status nux_capability_create(nux_u32 owner, nux_object_type type,
                                 nux_object_id object_id, nux_rights rights,
                                 nux_capability *out)
{
    if (!owner || type != NUX_OBJECT_PORT || object_id != registered_object ||
        !(rights & NUX_RIGHT_SEND) || !out)
        return NUX_E_INVALID;
    ++capability_creates;
    *out = next_capability++;
    return NUX_OK;
}


static nux_status task_create_status;
static nux_status elf_load_status;
static nux_status user_thread_status;
static nux_u32 destroyed_tasks;
static nux_task_info mock_task_info;
nux_status nux_task_get(nux_task_id task, nux_task_info *out)
{
    if (!task || !out) return NUX_E_INVALID;
    mock_task_info.id = task;
    mock_task_info.state = NUX_TASK_READY;
    *out = mock_task_info;
    return NUX_OK;
}
nux_status nux_task_create(const char *name, nux_u8 priority, nux_task_id *out)
{
    if (!name || priority != 31 || !out) return NUX_E_INVALID;
    if (task_create_status != NUX_OK) return task_create_status;
    *out = 99;
    return NUX_OK;
}
nux_status nux_elf_load(nux_task_id task, const void *image, nux_size size,
                        nux_elf_load_result *result)
{
    if (task != 99 || !image || !size || !result) return NUX_E_INVALID;
    if (elf_load_status != NUX_OK) return elf_load_status;
    result->entry = 0x400000;
    result->stack_pointer = 0x7fff0000;
    return NUX_OK;
}
nux_status nux_thread_create_user(nux_task_id task, nux_u64 entry,
                                  nux_u64 stack, nux_u64 argument,
                                  nux_u8 priority, nux_thread_id *out)
{
    (void)argument;
    if (task != 99 || entry != 0x400000 || stack != 0x7fff0000 ||
        priority != 31 || !out) return NUX_E_INVALID;
    if (user_thread_status != NUX_OK) return user_thread_status;
    *out = 123;
    return NUX_OK;
}
nux_status nux_task_destroy(nux_task_id task)
{
    if (task != 99) return NUX_E_NOT_FOUND;
    ++destroyed_tasks;
    return NUX_OK;
}

int main(void)
{
    nux_capability received = 0;
    if (nux_bootstrap_system_init() != NUX_OK) return 1;
    if (nux_bootstrap_register("", 7) != NUX_E_INVALID) return 2;
    if (nux_bootstrap_register("display", 7) != NUX_OK) return 3;
    if (nux_bootstrap_register("display", 7) != NUX_E_BUSY) return 4;
    current_task = 2;
    if (nux_bootstrap_lookup("display", &received) != NUX_OK ||
        received != 10 || capability_creates != 1) return 5;
    if (nux_bootstrap_unregister("display") != NUX_E_ACCESS) return 6;
    if (nux_bootstrap_lookup("missing", &received) != NUX_E_NOT_FOUND) return 7;
    current_task = 1;
    if (nux_bootstrap_unregister("display") != NUX_OK) return 8;
    current_task = 2;
    if (nux_bootstrap_lookup("display", &received) != NUX_E_NOT_FOUND) return 9;
    current_task = 1;
    if (nux_bootstrap_register("console", 8) != NUX_OK) return 10;
    if (nux_task_set_special_port(1, NUX_SPECIAL_PORT_TASK, 8) != NUX_OK) return 17;
    if (nux_task_get_special_port(1, NUX_SPECIAL_PORT_TASK, &received) != NUX_OK || received != 8) return 18;
    if (nux_task_set_special_port(1, 4, 8) != NUX_E_INVALID) return 19;
    if (nux_task_set_special_port(1, NUX_SPECIAL_PORT_TASK, 0) != NUX_OK) return 20;
    if (nux_task_get_special_port(1, NUX_SPECIAL_PORT_TASK, &received) != NUX_E_NOT_FOUND) return 21;
    nux_bootstrap_task_destroy(1);
    current_task = 2;
    if (nux_bootstrap_lookup("console", &received) != NUX_E_NOT_FOUND) return 11;
    if (nux_bootstrap_register("this-name-is-far-too-long-for-the-fixed-service-name-slot", 8)
        != NUX_E_INVALID) return 12;
    nux_u32 initial_task = 77;
    if (nux_bootstrap_start_initial_task(0, 128, &initial_task) != NUX_E_INVALID ||
        initial_task != 77) return 13;
    elf_load_status = NUX_E_INVALID;
    if (nux_bootstrap_start_initial_task(0x1000, 128, &initial_task) != NUX_E_INVALID ||
        initial_task != 0 || destroyed_tasks != 1) return 14;
    elf_load_status = NUX_OK;
    user_thread_status = NUX_E_NO_MEMORY;
    if (nux_bootstrap_start_initial_task(0x1000, 128, &initial_task) != NUX_E_NO_MEMORY ||
        destroyed_tasks != 2) return 15;
    user_thread_status = NUX_OK;
    if (nux_bootstrap_start_initial_task(0x1000, 128, &initial_task) != NUX_OK ||
        initial_task != 99) return 16;
    return 0;
}
