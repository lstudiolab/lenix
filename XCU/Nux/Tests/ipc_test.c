#include <nux/task.h>
#include <nux/capability.h>
#include <nux/port.h>

static nux_u64 test_next_space = 1;
nux_status nux_vm_space_create(nux_u64 *out)
{
    if (!out) return NUX_E_INVALID;
    *out = test_next_space++;
    return NUX_OK;
}
nux_status nux_vm_space_destroy(nux_u64 id)
{
    return id ? NUX_OK : NUX_E_INVALID;
}

nux_u32 nux_thread_task_busy(nux_task_id task)
{
    (void)task;
    return 0;
}

void nux_arch_lock(volatile nux_u32 *lock)
{
    while (*lock) { }
    *lock = 1;
}
void nux_arch_unlock(volatile nux_u32 *lock) { *lock = 0; }

int main(void)
{
    nux_task_id task, other_task;
    nux_capability send, receive, orphan_send, orphan_receive;
    nux_capability duplicate_send, send_once, moved_send;
    nux_object_id object_id;
    nux_port_set_id set;
    char input[] = "nux ipc";
    char output[16] = {0};
    nux_message message = {
        .id = 42, .kind = NUX_PORT_MSG_NORMAL,
        .inline_size = sizeof(input), .inline_data = input,
        .inline_capacity = sizeof(input)
    };

    if (nux_task_system_init() != NUX_OK) return 1;
    if (nux_capability_system_init() != NUX_OK) return 2;
    if (nux_port_system_init() != NUX_OK) return 3;
    if (nux_task_create("ipc-owner", 10, &task) != NUX_OK) return 4;
    if (nux_task_create("ipc-client", 10, &other_task) != NUX_OK) return 5;
    if (nux_port_create(task, 2, &send, &receive) != NUX_OK) return 6;
    if (nux_capability_check(task, receive, NUX_OBJECT_PORT,
                             NUX_RIGHT_CONTROL, &object_id) != NUX_OK) return 7;

    if (nux_port_send(other_task, send, &message, 0) != NUX_E_ACCESS) return 8;
    if (nux_port_send(task, send, &message, 0) != NUX_OK) return 9;
    if (nux_port_send(task, send, &message, 0) != NUX_OK) return 10;
    if (nux_port_send(task, send, &message, 0) != NUX_E_WOULD_BLOCK) return 11;

    nux_message received = {
        .inline_data = output, .inline_capacity = 2
    };
    if (nux_port_receive(task, receive, &received, 0) != NUX_E_LIMIT) return 12;
    received.inline_capacity = sizeof(output);
    if (nux_port_receive(task, receive, &received, 0) != NUX_OK) return 13;
    if (received.id != 42 || received.inline_size != sizeof(input)) return 14;
    for (unsigned long i = 0; i < sizeof(input); ++i)
        if (output[i] != input[i]) return 15;
    if (nux_port_receive(task, receive, &received, 0) != NUX_OK) return 16;

    if (nux_capability_create(task, NUX_OBJECT_PORT, object_id,
                              NUX_RIGHT_SEND_ONCE, &send_once) != NUX_OK) return 17;
    if (nux_port_send_once(task, send_once, &message) != NUX_OK) return 18;
    if (nux_port_send_once(task, send_once, &message) != NUX_E_ACCESS) return 19;
    if (nux_port_receive(task, receive, &received, 0) != NUX_OK) return 20;

    if (nux_port_set_create(task, &set) != NUX_OK) return 21;
    if (nux_port_set_add(task, set, receive) != NUX_OK) return 22;
    if (nux_port_set_receive(other_task, set, &received, 0) != NUX_E_ACCESS) return 23;
    if (nux_port_move_right(task, send, other_task,
                            NUX_PORT_RIGHT_SEND, &moved_send) != NUX_OK) return 24;
    if (nux_port_send(task, send, &message, 0) != NUX_E_ACCESS) return 25;
    if (nux_port_send(other_task, moved_send, &message, 0) != NUX_OK) return 26;
    if (nux_port_set_receive(task, set, &received, 0) != NUX_OK) return 27;

    if (nux_capability_create(task, NUX_OBJECT_PORT, object_id,
                              NUX_RIGHT_SEND, &duplicate_send) != NUX_OK) return 28;
    if (nux_port_destroy(task, receive) != NUX_OK) return 29;
    if (nux_port_send(task, duplicate_send, &message, 0) != NUX_E_ACCESS) return 30;
    if (nux_port_send(other_task, moved_send, &message, 0) != NUX_E_ACCESS) return 31;

    if (nux_port_create(task, 2, &orphan_send, &orphan_receive) != NUX_OK) return 32;
    if (nux_task_destroy(task) != NUX_OK) return 33;
    if (nux_port_send(task, orphan_send, &message, 0) != NUX_E_ACCESS) return 34;
    if (nux_port_set_receive(task, set, &received, 0) != NUX_E_NOT_FOUND) return 35;
    if (nux_task_destroy(other_task) != NUX_OK) return 36;
    return 0;
}
