#ifndef NUX_SYSCALL_H
#define NUX_SYSCALL_H
#include <nux/types.h>
#include <nux/status.h>
#define NUX_SYSCALL_ABI_VERSION 1u
#define NUX_SYSCALL_LIMIT 256u
#define NUX_SYSCALL_EXIT 0u
#define NUX_SYSCALL_YIELD 1u
#define NUX_SYSCALL_PORT_CREATE 2u
#define NUX_SYSCALL_PORT_SEND 3u
#define NUX_SYSCALL_PORT_RECEIVE 4u
#define NUX_SYSCALL_PORT_DESTROY 5u
#define NUX_SYSCALL_PORT_MAKE_SEND 6u
#define NUX_SYSCALL_PORT_MAKE_SEND_ONCE 7u
typedef nux_u64 nux_syscall_word;
typedef struct {
    nux_u32 id;
    nux_u32 kind;
    nux_u64 sequence;
    nux_u64 deadline_ns;
    nux_u32 inline_size;
    nux_u32 descriptor_count;
    nux_u64 inline_data;
    nux_u32 inline_capacity;
    nux_u32 reserved;
} nux_user_message;
typedef nux_status (*nux_syscall_handler)(nux_syscall_word,nux_syscall_word,nux_syscall_word,nux_syscall_word,nux_syscall_word,nux_syscall_word,nux_syscall_word *);
typedef struct { nux_u32 number; nux_u32 flags; nux_syscall_handler handler; } nux_syscall_entry;
nux_status nux_syscall_system_init(void);
nux_status nux_syscall_register(nux_u32 number, nux_syscall_handler handler, nux_u32 flags);
nux_status nux_syscall_dispatch(nux_u64 number, const nux_syscall_word arguments[6], nux_syscall_word *result);
nux_syscall_word nux_syscall_invoke(nux_u64 number, const nux_syscall_word arguments[6]);
extern volatile nux_u64 nux_last_user_exit_code;
#endif
