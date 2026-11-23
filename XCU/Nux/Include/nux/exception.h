#ifndef NUX_EXCEPTION_H
#define NUX_EXCEPTION_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/capability.h>
typedef enum { NUX_EXCEPTION_BAD_ACCESS=1, NUX_EXCEPTION_BAD_INSTRUCTION, NUX_EXCEPTION_ARITHMETIC, NUX_EXCEPTION_BREAKPOINT, NUX_EXCEPTION_GUARD, NUX_EXCEPTION_RESOURCE } nux_exception_kind;
typedef struct { nux_exception_kind kind; nux_u64 code; nux_u64 subcode; nux_u64 address; nux_u32 architecture_vector; nux_u32 reserved; } nux_exception;
typedef enum { NUX_EXCEPTION_DEFAULT=0, NUX_EXCEPTION_STATE, NUX_EXCEPTION_STATE_IDENTITY } nux_exception_behavior;
nux_status nux_exception_system_init(void);
nux_status nux_exception_set_handler(nux_u32 task_id, nux_capability handler, nux_exception_behavior behavior);
nux_status nux_exception_clear_handler(nux_u32 task_id);
nux_status nux_exception_deliver(nux_u32 thread_id, const nux_exception *exception);
nux_status nux_exception_return(nux_u32 thread_id, const nux_machine_state *state);
void nux_arch_exception_dispatch(nux_u64 vector, nux_u64 error, nux_u64 address);
#endif
