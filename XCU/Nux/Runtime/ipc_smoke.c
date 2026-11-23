#include <nux/port.h>
#include <nux/task.h>
#include <nux/thread.h>

static nux_task_id ipc_task;
static nux_capability ipc_send;
static nux_capability ipc_receive;
static volatile nux_u32 receiver_done;
static volatile nux_u32 sender_done;
static volatile nux_u32 ipc_failed;
static char received_bytes[32];

static const char sent_bytes[] = "Nux blocking IPC";

static void ipc_receiver(void *argument)
{
    (void)argument;
    nux_message message = {
        .inline_data = received_bytes,
        .inline_capacity = sizeof(received_bytes)
    };
    nux_status status = nux_port_receive_wait(
        ipc_task, ipc_receive, &message, 0);
    if (status != NUX_OK || message.id != 0x4e555849u ||
        message.inline_size != sizeof(sent_bytes)) {
        ipc_failed = 1;
    } else {
        for (nux_u32 i = 0; i < sizeof(sent_bytes); ++i)
            if (received_bytes[i] != sent_bytes[i]) ipc_failed = 1;
    }
    receiver_done = 1;
}

static void ipc_sender(void *argument)
{
    (void)argument;
    nux_message message = {
        .id = 0x4e555849u,
        .kind = NUX_PORT_MSG_NORMAL,
        .inline_size = sizeof(sent_bytes),
        .inline_data = (void *)sent_bytes,
        .inline_capacity = sizeof(sent_bytes)
    };
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
    for (nux_u32 i = 0; i < sizeof(received_bytes); ++i)
        received_bytes[i] = 0;

    status = nux_task_create("Nux blocking IPC smoke", 10, &ipc_task);
    if (status != NUX_OK) return status;
    status = nux_port_create(ipc_task, 4, &ipc_send, &ipc_receive);
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
