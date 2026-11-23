#include <nux/types.h>
#include <nux/status.h>
#include <nux/memory.h>
#include <nux/task.h>
#include <nux/thread.h>
#include <nux/capability.h>
#include <nux/port.h>
#include <nux/sync.h>
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
    for (;;) {
        if (nux_thread_yield() == NUX_E_WOULD_BLOCK)
            NUX_CPU_WAIT();
    }
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

    nux_boot_state = 0x4e55580000000008ULL;

    /*
     * Interrupts stay masked until architecture vector tables and timer
     * delivery are installed. The scheduler core is initialized but cannot
     * dispatch tasks until context-switch and interrupt-return paths exist.
     */
    for (;;) NUX_CPU_WAIT();
}
