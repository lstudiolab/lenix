#include <nux/boot.h>
#define NUX_BOOT_STATUS_MAGIC 0x4e55580000000000ULL
volatile nux_u64 nux_boot_status;

#if defined(NUX_KERNEL) && defined(__aarch64__) && defined(NUX_QEMU_SEMIHOST)
static void nux_boot_debug_write(const char *text)
{
    register nux_u64 operation __asm__("x0") = 0x04;
    register const char *message __asm__("x1") = text;
    __asm__ volatile("hlt #0xf000" : "+r"(operation) : "r"(message) : "memory");
}
#elif defined(NUX_KERNEL) && defined(__x86_64__)
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
    switch (phase) {
    case NUX_BOOT_PHASE_ENTRY: nux_boot_debug_write("NUX:ENTRY\n"); break;
    case NUX_BOOT_PHASE_MEMORY: nux_boot_debug_write("NUX:MEMORY\n"); break;
    case NUX_BOOT_PHASE_ACPI: nux_boot_debug_write("NUX:ACPI\n"); break;
    case NUX_BOOT_PHASE_PROCESSORS: nux_boot_debug_write("NUX:PROCESSORS\n"); break;
    case NUX_BOOT_PHASE_VIRTUAL_MEMORY: nux_boot_debug_write("NUX:VM\n"); break;
    case NUX_BOOT_PHASE_TASKS: nux_boot_debug_write("NUX:TASKS\n"); break;
    case NUX_BOOT_PHASE_CAPABILITIES: nux_boot_debug_write("NUX:CAPABILITIES\n"); break;
    case NUX_BOOT_PHASE_THREADS: nux_boot_debug_write("NUX:THREADS\n"); break;
    case NUX_BOOT_PHASE_IPC: nux_boot_debug_write("NUX:IPC\n"); break;
    case NUX_BOOT_PHASE_SYNCHRONIZATION: nux_boot_debug_write("NUX:SYNC\n"); break;
    case NUX_BOOT_PHASE_SYSCALLS: nux_boot_debug_write("NUX:SYSCALLS\n"); break;
    case NUX_BOOT_PHASE_TIME: nux_boot_debug_write("NUX:TIME\n"); break;
    case NUX_BOOT_PHASE_USER_EXIT: nux_boot_debug_write("NUX:USER_EXIT\n"); break;
    case NUX_BOOT_PHASE_READY: nux_boot_debug_write("NUX:READY\n"); break;
    default: break;
    }
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
