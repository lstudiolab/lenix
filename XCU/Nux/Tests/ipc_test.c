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

nux_u32 nux_thread_task_busy(nux_task_id task){(void)task;return 0;}

void nux_arch_lock(volatile nux_u32 *lock)
{
    while (*lock) { }
    *lock = 1;
}

void nux_arch_unlock(volatile nux_u32 *lock) { *lock = 0; }

int main(void)
{
    nux_task_id task;
    nux_capability send, receive, orphan_send, orphan_receive, duplicate_send, send_once;
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
    if (nux_task_create("ipc-test", 10, &task) != NUX_OK) return 4;
    if (nux_port_create(task, 2, &send, &receive) != NUX_OK) return 5;
    if (nux_capability_check(receive, NUX_OBJECT_PORT, NUX_RIGHT_CONTROL, &object_id) != NUX_OK) return 31;
    if (nux_port_send(send, &message, 0) != NUX_OK) return 6;
    if (nux_port_send(send, &message, 0) != NUX_OK) return 17;
    if (nux_port_send(send, &message, 0) != NUX_E_WOULD_BLOCK) return 18;

    nux_message received = {
        .inline_data = output, .inline_capacity = 2
    };
    if (nux_port_receive(receive, &received, 0) != NUX_E_LIMIT) return 7;

    received.inline_capacity = sizeof(output);
    if (nux_port_receive(receive, &received, 0) != NUX_OK) return 8;
    if (received.id != 42 || received.inline_size != sizeof(input)) return 9;
    for (unsigned long i = 0; i < sizeof(input); ++i)
        if (output[i] != input[i]) return 10;
    received.inline_capacity = sizeof(output);
    if (nux_port_receive(receive, &received, 0) != NUX_OK) return 19;
    if (nux_capability_create(NUX_OBJECT_PORT, object_id, NUX_RIGHT_SEND_ONCE, &send_once) != NUX_OK) return 27;
    if (nux_port_send_once(send_once, &message) != NUX_OK) return 28;
    if (nux_port_send_once(send_once, &message) != NUX_E_ACCESS) return 29;
    received.inline_capacity = sizeof(output);
    if (nux_port_receive(receive, &received, 0) != NUX_OK) return 30;

    if (nux_port_set_create(task, &set) != NUX_OK) return 11;
    if (nux_port_set_add(set, receive) != NUX_OK) return 12;
    if (nux_port_send(send, &message, 0) != NUX_OK) return 13;
    received.inline_capacity = sizeof(output);
    if (nux_port_set_receive(set, &received, 0) != NUX_OK) return 14;

    if (nux_capability_check(receive, NUX_OBJECT_PORT, NUX_RIGHT_CONTROL, &object_id) != NUX_OK) return 15;
    if (nux_capability_create(NUX_OBJECT_PORT, object_id, NUX_RIGHT_SEND, &duplicate_send) != NUX_OK) return 24;
    if (nux_port_destroy(receive) != NUX_OK) return 25;
    if (nux_port_send(send, &message, 0) != NUX_E_ACCESS) return 16;
    if (nux_port_send(duplicate_send, &message, 0) != NUX_E_ACCESS) return 26;
    if (nux_port_create(task, 2, &orphan_send, &orphan_receive) != NUX_OK) return 20;
    if (nux_task_destroy(task) != NUX_OK) return 21;
    if (nux_port_send(orphan_send, &message, 0) != NUX_E_ACCESS) return 22;
    if (nux_port_set_receive(set, &received, 0) != NUX_E_NOT_FOUND) return 23;
    return 0;
}
