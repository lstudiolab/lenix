#include <nux/boot.h>
#define NUX_BOOT_STATUS_MAGIC 0x4e55580000000000ULL
volatile nux_u64 nux_boot_status;

#if defined(NUX_KERNEL) && defined(__x86_64__)
static void nux_boot_debug_write(const char *text)
{
    while (*text) {
        nux_u8 value = (nux_u8)*text++;
        __asm__ volatile("outb %0, $0xe9" : : "a"(value));
    }
}
#else
static void nux_boot_debug_write(const char *text) { (void)text; }
#endif

void nux_boot_mark(nux_boot_phase phase)
{
    nux_boot_status = NUX_BOOT_STATUS_MAGIC | ((nux_u64)phase << 32);
    __asm__ volatile("" ::: "memory");
    if (phase == NUX_BOOT_PHASE_READY)
        nux_boot_debug_write("NUX:READY\n");
}

__attribute__((noreturn))
void nux_boot_fail(nux_u32 code)
{
    nux_boot_status = NUX_BOOT_STATUS_MAGIC |
        ((nux_u64)NUX_BOOT_PHASE_FAILED << 32) | code;
    __asm__ volatile("" ::: "memory");
    nux_boot_debug_write("NUX:FAIL\n");
#if defined(__aarch64__)
    for (;;) __asm__ volatile("msr daifset, #0xf; wfe" ::: "memory");
#elif defined(__x86_64__)
    for (;;) __asm__ volatile("cli; hlt" ::: "memory");
#else
#error Unsupported Nux architecture
#endif
}
