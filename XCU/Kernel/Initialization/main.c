#include <nux/types.h>
#include <nux/status.h>
#include <nux/memory.h>
#include <nux/task.h>
#include <nux/thread.h>
#include <nux/capability.h>
#include <nux/port.h>
#include <nux/sync.h>
#include <nux/syscall.h>
#include <nux/vm.h>
#include <nux/time.h>
#include "entry.h"

#if defined(__aarch64__)
#define NUX_CPU_WAIT() __asm__ volatile("wfe")
#elif defined(__x86_64__)
#define NUX_CPU_WAIT() __asm__ volatile("hlt")
#else
#error Unsupported Nux architecture
#endif

static volatile nux_u64 nux_boot_state;

static void nux_boot_failure(nux_u64 code)
{
    nux_boot_state = 0x4e55580000000000ULL | code;
    for (;;) NUX_CPU_WAIT();
}

void nux_kernel_main(const nux_boot_info *boot_info)
{
    if (boot_info == NUX_NULL)
        nux_boot_failure(1);
    if (nux_memory_init(boot_info) != NUX_OK)
        nux_boot_failure(2);
    if (nux_task_system_init() != NUX_OK)
        nux_boot_failure(3);
    if (nux_capability_system_init() != NUX_OK)
        nux_boot_failure(4);
    if (nux_thread_system_init() != NUX_OK)
        nux_boot_failure(5);
    if (nux_port_system_init() != NUX_OK)
        nux_boot_failure(6);
    if (nux_sync_system_init() != NUX_OK)
        nux_boot_failure(7);
    if (nux_syscall_system_init() != NUX_OK)
        nux_boot_failure(8);
    if (nux_vm_system_init() != NUX_OK)
        nux_boot_failure(9);
    (void)nux_time_system_init();

    nux_boot_state = 0x4e5558000000000bULL;

    /*
     * Interrupts remain masked until timer and interrupt-controller support
     * is ready. Kernel threads currently run cooperatively; user dispatch
     * and preemption remain disabled.
     */
    for (;;) {
        if (nux_thread_yield() == NUX_E_WOULD_BLOCK)
            NUX_CPU_WAIT();
    }
}
