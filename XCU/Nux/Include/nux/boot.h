#ifndef NUX_BOOT_H
#define NUX_BOOT_H
#include <nux/types.h>
typedef enum nux_boot_phase {
    NUX_BOOT_PHASE_ENTRY = 1, NUX_BOOT_PHASE_MEMORY,
    NUX_BOOT_PHASE_ACPI, NUX_BOOT_PHASE_PROCESSORS,
    NUX_BOOT_PHASE_VIRTUAL_MEMORY, NUX_BOOT_PHASE_TASKS,
    NUX_BOOT_PHASE_CAPABILITIES, NUX_BOOT_PHASE_THREADS,
    NUX_BOOT_PHASE_IPC, NUX_BOOT_PHASE_SYNCHRONIZATION,
    NUX_BOOT_PHASE_SYSCALLS, NUX_BOOT_PHASE_TIME,
    NUX_BOOT_PHASE_READY, NUX_BOOT_PHASE_FAILED = 0xff
} nux_boot_phase;
extern volatile nux_u64 nux_boot_status;
void nux_boot_mark(nux_boot_phase phase);
__attribute__((noreturn)) void nux_boot_fail(nux_u32 code);
#endif
