#include <nux/types.h>
#include <nux/exception.h>
#include <nux/user.h>
#include <nux/types.h>

#if defined(__x86_64__)
    if (frame && (((nux_x86_exception_frame *)frame)->cs & 3u) == 3u) {
        (void)nux_thread_exit_current();
        for (;;) __asm__ volatile("cli; hlt" ::: "memory");
    }
    if (vector >= 32) return;
    for (;;) __asm__ volatile("cli; hlt" ::: "memory");
#elif defined(__aarch64__)
    if (frame && ((((nux_arm64_exception_frame *)frame)->spsr & 0xfu) == 0u)) {
        (void)nux_thread_exit_current();
        for (;;) __asm__ volatile("msr daifset, #0xf; wfe" ::: "memory");
    }
    for (;;) __asm__ volatile("msr daifset, #0xf; wfe" ::: "memory");
#endif
}
