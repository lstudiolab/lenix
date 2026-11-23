#include <nux/types.h>
#include <nux/status.h>
#include <nux/boot.h>
#include <nux/memory.h>
#include <nux/task.h>
#include <nux/thread.h>
#include <nux/capability.h>
#include <nux/port.h>
#include <nux/bootstrap.h>
#include <nux/sync.h>
#include <nux/syscall.h>
#include <nux/vm.h>
#include <nux/time.h>
#include <nux/acpi.h>
#include <nux/processor.h>

extern nux_status nux_ipc_smoke_test(void);
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
    if (boot_info->signature != NUX_BOOT_INFO_SIGNATURE ||
        boot_info->version != NUX_BOOT_INFO_VERSION ||
        boot_info->structure_size < sizeof(*boot_info) ||
        boot_info->memory_map == 0 || boot_info->memory_map_count == 0 ||
        boot_info->hhdm_offset == 0 ||
        boot_info->direct_map_base != boot_info->hhdm_offset ||
        boot_info->direct_map_size < (64ULL * 1024ULL * 1024ULL * 1024ULL) ||
        (boot_info->flags & NUX_BOOT_FLAG_IMAGE_VALIDATED) == 0)
        nux_boot_fail(13);
#if defined(__x86_64__)
    if (boot_info->architecture != NUX_BOOT_ARCH_X86_64) nux_boot_fail(14);
#elif defined(__aarch64__)
    if (boot_info->architecture != NUX_BOOT_ARCH_AARCH64) nux_boot_fail(14);
#endif
    if (boot_info->kernel_virtual_base != 0xffffffff80000000ULL ||
        boot_info->kernel_image_size == 0 ||
        boot_info->kernel_physical_base >= 0x100000000ULL)
        nux_boot_fail(15);
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
    if (nux_bootstrap_system_init() != NUX_OK) nux_boot_fail(16);
    nux_boot_mark(NUX_BOOT_PHASE_SYNCHRONIZATION);
    if (nux_sync_system_init() != NUX_OK) nux_boot_fail(8);
    nux_boot_mark(NUX_BOOT_PHASE_SYSCALLS);
    if (nux_syscall_system_init() != NUX_OK) nux_boot_fail(9);
    nux_boot_mark(NUX_BOOT_PHASE_TIME);
    (void)nux_time_system_init();
    if (nux_user_smoke_test() != NUX_OK) nux_boot_fail(11);
    if (nux_ipc_smoke_test() != NUX_OK) nux_boot_fail(12);
    nux_boot_mark(NUX_BOOT_PHASE_READY);

    /* Keep interrupts masked until timer and interrupt-controller support exists. */
    for (;;) {
        if (nux_thread_yield() == NUX_E_WOULD_BLOCK) NUX_CPU_WAIT();
    }
}
