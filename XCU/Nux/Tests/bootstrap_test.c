#include <nux/bootstrap.h>
#include <nux/thread.h>
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
    nux_bootstrap_task_destroy(1);
    current_task = 2;
    if (nux_bootstrap_lookup("console", &received) != NUX_E_NOT_FOUND) return 11;
    if (nux_bootstrap_register("this-name-is-far-too-long-for-the-fixed-service-name-slot", 8)
        != NUX_E_INVALID) return 12;
    return 0;
}
