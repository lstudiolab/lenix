#ifndef NUX_PORT_H
#define NUX_PORT_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/object.h>
#include <nux/capability.h>
#include <nux/task.h>
#define NUX_PORT_LIMIT 1024u
#define NUX_PORT_SET_LIMIT 128u
#define NUX_PORT_QUEUE_DEFAULT 64u
#define NUX_MESSAGE_INLINE_MAX 4096u
typedef nux_u32 nux_port_name;
typedef nux_u64 nux_port_set_id;
typedef enum { NUX_PORT_MSG_NORMAL=0, NUX_PORT_MSG_NOTIFY, NUX_PORT_MSG_RPC } nux_message_kind;
/* inline_capacity is supplied by receivers; nonzero wait timeouts are not yet supported. */
typedef struct { nux_u32 id; nux_u32 kind; nux_u64 sequence; nux_u64 deadline_ns; nux_u32 inline_size; nux_u32 descriptor_count; void *inline_data; nux_u32 inline_capacity; } nux_message;
typedef enum { NUX_PORT_RIGHT_SEND=1, NUX_PORT_RIGHT_RECEIVE=2, NUX_PORT_RIGHT_SEND_ONCE=3 } nux_port_right_kind;
nux_status nux_port_system_init(void);
nux_status nux_port_task_destroy(nux_task_id owner);
nux_status nux_port_create(nux_task_id owner, nux_u32 queue_limit, nux_capability *send, nux_capability *receive);
nux_status nux_port_make_send(nux_task_id owner, nux_capability receive, nux_capability *out);
nux_status nux_port_copy_send(nux_task_id owner, nux_capability send, nux_capability *out);
nux_status nux_port_make_send_once(nux_task_id owner, nux_capability receive, nux_capability *out);
nux_status nux_port_destroy(nux_task_id owner, nux_capability control);
nux_status nux_port_send(nux_task_id sender, nux_capability send, const nux_message *message, nux_u64 timeout_ns);
nux_status nux_port_send_wait(nux_task_id sender, nux_capability send, const nux_message *message, nux_u64 timeout_ns);
nux_status nux_port_send_once(nux_task_id sender, nux_capability send_once, const nux_message *message);
nux_status nux_port_receive(nux_task_id receiver, nux_capability receive, nux_message *message, nux_u64 timeout_ns);
nux_status nux_port_receive_wait(nux_task_id receiver, nux_capability receive, nux_message *message, nux_u64 timeout_ns);
nux_status nux_port_move_right(nux_task_id source_owner, nux_capability source, nux_task_id destination, nux_port_right_kind right, nux_capability *out);
nux_status nux_port_set_create(nux_task_id owner, nux_port_set_id *out);
nux_status nux_port_set_add(nux_task_id owner, nux_port_set_id set, nux_capability receive);
nux_status nux_port_set_remove(nux_task_id owner, nux_port_set_id set, nux_capability receive);
nux_status nux_port_set_receive(nux_task_id owner, nux_port_set_id set, nux_message *message, nux_u64 timeout_ns);
nux_status nux_port_set_receive_wait(nux_task_id owner, nux_port_set_id set, nux_message *message, nux_u64 timeout_ns);
nux_status nux_port_request_notification(nux_task_id owner, nux_capability receive, nux_u32 event, nux_capability notify_port);
#endif
