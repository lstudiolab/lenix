#include <nux/boot.h>
#define NUX_BOOT_STATUS_MAGIC 0x4e55580000000000ULL
volatile nux_u64 nux_boot_status;
void nux_boot_mark(nux_boot_phase phase)
{
    nux_boot_status = NUX_BOOT_STATUS_MAGIC | ((nux_u64)phase << 32);
    __asm__ volatile("" ::: "memory");
}
__attribute__((noreturn))
void nux_boot_fail(nux_u32 code)
{
    nux_boot_status = NUX_BOOT_STATUS_MAGIC |
        ((nux_u64)NUX_BOOT_PHASE_FAILED << 32) | code;
    __asm__ volatile("" ::: "memory");
#if defined(__aarch64__)
    for (;;) __asm__ volatile("msr daifset, #0xf; wfe" ::: "memory");
#elif defined(__x86_64__)
    for (;;) __asm__ volatile("cli; hlt" ::: "memory");
#else
#error Unsupported Nux architecture
#endif
}
