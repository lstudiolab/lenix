#include <nux/port.h>
#include <nux/task.h>
#include <nux/thread.h>

static nux_task_id ipc_task;
static nux_capability ipc_send;
static nux_capability ipc_receive;
static nux_port_set_id ipc_set;
static volatile nux_u32 receiver_done;
static volatile nux_u32 sender_done;
static volatile nux_u32 ipc_failed;
static char direct_bytes[32];
static char set_bytes[32];

static const char direct_sent[] = "Direct blocking IPC";
static const char set_sent[] = "Port-set blocking IPC";

static nux_u32 message_matches(const nux_message *message, nux_u32 id,
                               const char *expected, nux_u32 expected_size)
{
    if (message->id != id || message->inline_size != expected_size)
        return 0;
    const char *actual = (const char *)message->inline_data;
    for (nux_u32 i = 0; i < expected_size; ++i)
        if (actual[i] != expected[i]) return 0;
    return 1;
}

static void ipc_receiver(void *argument)
{
    (void)argument;
    nux_message message = {
        .inline_data = direct_bytes,
        .inline_capacity = sizeof(direct_bytes)
    };
    nux_status status = nux_port_receive_wait(
        ipc_task, ipc_receive, &message, 0);
    if (status != NUX_OK ||
        !message_matches(&message, 0x4e555844u, direct_sent,
                         sizeof(direct_sent)))
        ipc_failed = 1;

    message.inline_data = set_bytes;
    message.inline_capacity = sizeof(set_bytes);
    status = nux_port_set_receive_wait(ipc_task, ipc_set, &message, 0);
    if (status != NUX_OK ||
        !message_matches(&message, 0x4e555853u, set_sent,
                         sizeof(set_sent)))
        ipc_failed = 1;
    receiver_done = 1;
}

static void ipc_sender(void *argument)
{
    (void)argument;
    nux_message message = {
        .id = 0x4e555844u,
        .kind = NUX_PORT_MSG_NORMAL,
        .inline_size = sizeof(direct_sent),
        .inline_data = (void *)direct_sent,
        .inline_capacity = sizeof(direct_sent)
    };
    if (nux_port_send(ipc_task, ipc_send, &message, 0) != NUX_OK)
        ipc_failed = 1;
    if (nux_thread_yield() != NUX_OK) ipc_failed = 1;

    message.id = 0x4e555853u;
    message.inline_size = sizeof(set_sent);
    message.inline_data = (void *)set_sent;
    message.inline_capacity = sizeof(set_sent);
    if (nux_port_send(ipc_task, ipc_send, &message, 0) != NUX_OK)
        ipc_failed = 1;
    sender_done = 1;
}

nux_status nux_ipc_smoke_test(void)
{
    nux_thread_id receiver = 0, sender = 0;
    nux_status status;

    receiver_done = 0;
    sender_done = 0;
    ipc_failed = 0;
    for (nux_u32 i = 0; i < sizeof(direct_bytes); ++i) direct_bytes[i] = 0;
    for (nux_u32 i = 0; i < sizeof(set_bytes); ++i) set_bytes[i] = 0;

    status = nux_task_create("Nux blocking IPC smoke", 10, &ipc_task);
    if (status != NUX_OK) return status;
    status = nux_port_create(ipc_task, 4, &ipc_send, &ipc_receive);
    if (status != NUX_OK) goto failed;
    status = nux_port_set_create(ipc_task, &ipc_set);
    if (status != NUX_OK) goto failed;
    status = nux_port_set_add(ipc_task, ipc_set, ipc_receive);
    if (status != NUX_OK) goto failed;
    status = nux_thread_create_kernel(ipc_task, ipc_receiver, NUX_NULL,
                                     10, &receiver);
    if (status != NUX_OK) goto failed;
    status = nux_thread_create_kernel(ipc_task, ipc_sender, NUX_NULL,
                                     10, &sender);
    if (status != NUX_OK) goto failed;

    status = nux_thread_yield();
    if (status != NUX_OK) goto failed;
    if (!receiver_done || !sender_done || ipc_failed) {
        status = NUX_E_BAD_STATE;
        goto failed;
    }
    return nux_task_destroy(ipc_task);

failed:
    if (receiver) (void)nux_thread_terminate(receiver);
    if (sender) (void)nux_thread_terminate(sender);
    if (ipc_task) (void)nux_task_destroy(ipc_task);
    return status;
}
