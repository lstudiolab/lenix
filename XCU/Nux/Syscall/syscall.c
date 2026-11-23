#include <nux/syscall.h>
#include <nux/thread.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

volatile nux_u64 nux_last_user_exit_code;
static nux_syscall_entry calls[NUX_SYSCALL_LIMIT];
static volatile nux_u32 syscall_lock;


static nux_status syscall_exit_handler(nux_syscall_word a, nux_syscall_word b,
                                        nux_syscall_word c, nux_syscall_word d,
                                        nux_syscall_word e, nux_syscall_word f,
                                        nux_syscall_word *result)
{
    nux_last_user_exit_code = a;
    (void)b; (void)c; (void)d; (void)e; (void)f;
    *result = 0;
    return nux_thread_exit_current();
}

static nux_status syscall_yield_handler(nux_syscall_word a, nux_syscall_word b,
                                        nux_syscall_word c, nux_syscall_word d,
                                        nux_syscall_word e, nux_syscall_word f,
                                        nux_syscall_word *result)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    *result = 0;
    return nux_thread_yield();
}

nux_status nux_syscall_system_init(void)
{
    nux_arch_lock(&syscall_lock);
    for (nux_u32 i = 0; i < NUX_SYSCALL_LIMIT; ++i) {
        calls[i].number = i;
        calls[i].flags = 0;
        calls[i].handler = NUX_NULL;
    }
    nux_arch_unlock(&syscall_lock);
    nux_last_user_exit_code = 0;
    nux_status status = nux_syscall_register(NUX_SYSCALL_EXIT, syscall_exit_handler, 0);
    if (status != NUX_OK) return status;
    return nux_syscall_register(NUX_SYSCALL_YIELD, syscall_yield_handler, 0);
}

nux_status nux_syscall_register(nux_u32 number, nux_syscall_handler handler,
                                nux_u32 flags)
{
    if (number >= NUX_SYSCALL_LIMIT || !handler)
        return NUX_E_INVALID;
    nux_arch_lock(&syscall_lock);
    if (calls[number].handler) {
        nux_arch_unlock(&syscall_lock);
        return NUX_E_BUSY;
    }
    calls[number].flags = flags;
    calls[number].handler = handler;
    nux_arch_unlock(&syscall_lock);
    return NUX_OK;
}

nux_status nux_syscall_dispatch(nux_u64 number,
                                const nux_syscall_word arguments[6],
                                nux_syscall_word *result)
{
    nux_syscall_handler handler;
    if (number >= NUX_SYSCALL_LIMIT || !arguments || !result)
        return NUX_E_INVALID;
    nux_arch_lock(&syscall_lock);
    handler = calls[number].handler;
    nux_arch_unlock(&syscall_lock);
    if (!handler) return NUX_E_NOT_SUPPORTED;
    *result = 0;
    return handler(arguments[0], arguments[1], arguments[2],
                   arguments[3], arguments[4], arguments[5], result);
}

nux_syscall_word nux_syscall_invoke(nux_u64 number,
                                    const nux_syscall_word arguments[6])
{
    nux_syscall_word result = 0;
    if (number >= NUX_SYSCALL_LIMIT)
        return (nux_syscall_word)(nux_i64)NUX_E_INVALID;
    nux_status status = nux_syscall_dispatch(number, arguments, &result);
    return status == NUX_OK ? result : (nux_syscall_word)(nux_i64)status;
}
