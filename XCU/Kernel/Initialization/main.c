#include <nux/types.h>
#include <nux/status.h>
#include <nux/boot.h>
#include <nux/memory.h>
#include <nux/task.h>
#include <nux/thread.h>
#include <nux/capability.h>
#include <nux/port.h>
#include <nux/sync.h>
#include <nux/syscall.h>
#include <nux/vm.h>
#include <nux/time.h>
#include <nux/acpi.h>
#include <nux/processor.h>
#include <nux/user.h>
#include "entry.h"

#if defined(__aarch64__)
#define NUX_CPU_WAIT() __asm__ volatile("wfe")
#elif defined(__x86_64__)
#define NUX_CPU_WAIT() __asm__ volatile("hlt")
#else
#error Unsupported Nux architecture
#endif

void nux_kernel_main(const nux_boot_info *boot_info)
{
    nux_boot_mark(NUX_BOOT_PHASE_ENTRY);
    if (boot_info == NUX_NULL) nux_boot_fail(1);
    nux_boot_mark(NUX_BOOT_PHASE_MEMORY);
    if (nux_memory_init(boot_info) != NUX_OK) nux_boot_fail(2);
    nux_boot_mark(NUX_BOOT_PHASE_ACPI);
    /* ACPI is optional at this stage; retain topology when firmware provides it. */
    (void)nux_acpi_system_init((const void *)(nux_uptr)boot_info->firmware_table);
    nux_boot_mark(NUX_BOOT_PHASE_PROCESSORS);
    if (nux_processor_system_init() != NUX_OK) nux_boot_fail(10);
    nux_boot_mark(NUX_BOOT_PHASE_VIRTUAL_MEMORY);
    if (nux_vm_system_init() != NUX_OK) nux_boot_fail(3);
    nux_boot_mark(NUX_BOOT_PHASE_TASKS);
    if (nux_task_system_init() != NUX_OK) nux_boot_fail(4);
    nux_boot_mark(NUX_BOOT_PHASE_CAPABILITIES);
    if (nux_capability_system_init() != NUX_OK) nux_boot_fail(5);
    nux_boot_mark(NUX_BOOT_PHASE_THREADS);
    if (nux_thread_system_init() != NUX_OK) nux_boot_fail(6);
    nux_boot_mark(NUX_BOOT_PHASE_IPC);
    if (nux_port_system_init() != NUX_OK) nux_boot_fail(7);
    nux_boot_mark(NUX_BOOT_PHASE_SYNCHRONIZATION);
    if (nux_sync_system_init() != NUX_OK) nux_boot_fail(8);
    nux_boot_mark(NUX_BOOT_PHASE_SYSCALLS);
    if (nux_syscall_system_init() != NUX_OK) nux_boot_fail(9);
    nux_boot_mark(NUX_BOOT_PHASE_TIME);
    (void)nux_time_system_init();
    if (nux_user_smoke_test() != NUX_OK) nux_boot_fail(11);
    nux_boot_mark(NUX_BOOT_PHASE_READY);

    /* Keep interrupts masked until timer and interrupt-controller support exists. */
    for (;;) {
        if (nux_thread_yield() == NUX_E_WOULD_BLOCK) NUX_CPU_WAIT();
    }
}
