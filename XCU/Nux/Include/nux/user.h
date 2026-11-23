#ifndef NUX_USER_H
#define NUX_USER_H
#include <nux/types.h>
#include <nux/status.h>
void nux_arch_user_init(void);
void nux_arch_set_kernel_stack(nux_u64 stack_top);
__attribute__((noreturn)) void nux_arch_user_enter(nux_u64 entry,
                                                    nux_u64 stack_pointer,
                                                    nux_u64 argument,
                                                    nux_u64 kernel_stack_top);
nux_status nux_user_smoke_test(void);
#endif
