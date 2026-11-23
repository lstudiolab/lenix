#include <nux/syscall.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

static nux_syscall_entry calls[NUX_SYSCALL_LIMIT];
static volatile nux_u32 syscall_lock;

nux_status nux_syscall_system_init(void)
{
    nux_arch_lock(&syscall_lock);
    for (nux_u32 i = 0; i < NUX_SYSCALL_LIMIT; ++i) {
        calls[i].number = i;
        calls[i].flags = 0;
        calls[i].handler = NUX_NULL;
    }
    nux_arch_unlock(&syscall_lock);
    return NUX_OK;
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

nux_status nux_syscall_dispatch(nux_u32 number,
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
