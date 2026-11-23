#ifndef NUX_SYSCALL_H
#define NUX_SYSCALL_H
#include <nux/types.h>
#include <nux/status.h>
#define NUX_SYSCALL_ABI_VERSION 1u
#define NUX_SYSCALL_LIMIT 256u
#define NUX_SYSCALL_EXIT 0u
#define NUX_SYSCALL_YIELD 1u
typedef nux_u64 nux_syscall_word;
typedef nux_status (*nux_syscall_handler)(nux_syscall_word,nux_syscall_word,nux_syscall_word,nux_syscall_word,nux_syscall_word,nux_syscall_word,nux_syscall_word *);
typedef struct { nux_u32 number; nux_u32 flags; nux_syscall_handler handler; } nux_syscall_entry;
nux_status nux_syscall_system_init(void);
nux_status nux_syscall_register(nux_u32 number, nux_syscall_handler handler, nux_u32 flags);
nux_status nux_syscall_dispatch(nux_u32 number, const nux_syscall_word arguments[6], nux_syscall_word *result);
nux_syscall_word nux_syscall_invoke(nux_u64 number, const nux_syscall_word arguments[6]);
void nux_arch_syscall_entry(void);
#endif
