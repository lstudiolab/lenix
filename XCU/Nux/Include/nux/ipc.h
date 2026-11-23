#ifndef NUX_IPC_H
#define NUX_IPC_H

#include <nux/types.h>
#include <nux/object.h>
#include <nux/status.h>

#define NUX_IPC_INLINE_MAX 256u
#define NUX_IPC_MAX_RIGHTS 8u

typedef nux_u64 nux_port_name;
typedef nux_u64 nux_deadline;

/* Versioned message header; payload interpretation is defined by message_id. */
typedef struct nux_message_header {
    nux_u32 size;
    nux_u16 version;
    nux_u16 flags;
    nux_u64 message_id;
    nux_port_name destination;
    nux_u64 correlation_id;
} nux_message_header;

typedef struct nux_message {
    nux_message_header header;
    nux_u32 inline_size;
    nux_u16 transferred_right_count;
    nux_u16 reserved;
    nux_u8 inline_data[NUX_IPC_INLINE_MAX];
} nux_message;

enum {
    NUX_IPC_SEND          = 1u << 0,
    NUX_IPC_RECEIVE       = 1u << 1,
    NUX_IPC_NONBLOCK      = 1u << 2,
    NUX_IPC_MOVE_RIGHTS   = 1u << 3
};

/*
 * This is an internal kernel contract, not yet a userspace syscall ABI.
 * User pointers must be copied/validated by the eventual syscall layer.
 */
nux_status nux_ipc_send(nux_port_name port,
                        const nux_message *message,
                        nux_deadline deadline);
nux_status nux_ipc_receive(nux_port_name port,
                           nux_message *message,
                           nux_deadline deadline);

#endif
