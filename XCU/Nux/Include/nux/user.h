#ifndef NUX_USER_H
#define NUX_USER_H
#include <nux/types.h>
void nux_arch_user_init(void);
void nux_arch_user_set_kernel_stack(nux_u64 stack_top);
__attribute__((noreturn)) void nux_arch_enter_user(nux_u64 entry,
                                                   nux_u64 stack_pointer);
#endif
