#include <nux/syscall.h>

void nux_arch_lock(volatile nux_u32 *lock)
{
    while (*lock) { }
    *lock = 1;
}
void nux_arch_unlock(volatile nux_u32 *lock) { *lock = 0; }
nux_status nux_thread_yield(void) { return NUX_OK; }
nux_status nux_thread_exit_current(void) { return NUX_OK; }

static nux_status add_handler(nux_syscall_word a, nux_syscall_word b,
                              nux_syscall_word c, nux_syscall_word d,
                              nux_syscall_word e, nux_syscall_word f,
                              nux_syscall_word *result)
{
    *result = a + b + c + d + e + f;
    return NUX_OK;
}

int main(void)
{
    nux_syscall_word args[6] = {1, 2, 3, 4, 5, 6};
    nux_syscall_word result = 99;
    if (nux_syscall_system_init() != NUX_OK) return 1;
    if (nux_syscall_dispatch(7, args, &result) != NUX_E_NOT_SUPPORTED) return 2;
    if (nux_syscall_dispatch(NUX_SYSCALL_YIELD, args, &result) != NUX_OK) return 8;
    if (nux_syscall_invoke(NUX_SYSCALL_YIELD, args) != 0) return 9;
    if ((nux_i64)nux_syscall_invoke(999, args) != NUX_E_INVALID) return 10;
    if (nux_syscall_register(7, add_handler, 0) != NUX_OK) return 3;
    if (nux_syscall_register(7, add_handler, 0) != NUX_E_BUSY) return 4;
    if (nux_syscall_dispatch(7, args, &result) != NUX_OK) return 5;
    if (result != 21) return 6;
    if (nux_syscall_register(NUX_SYSCALL_LIMIT, add_handler, 0) != NUX_E_INVALID) return 7;
    return 0;
}
