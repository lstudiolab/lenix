#include <nux/types.h>
#include <nux/status.h>
#include "../Entry/entry.h"
#include "../Boot/limine.h"

static volatile nux_u64 nux_boot_state;

void nux_kernel_main(const nux_boot_info *boot_info)
{
    if (boot_info == NUX_NULL || boot_info->memory_map == 0 ||
        boot_info->memory_map_count == 0) {
        nux_boot_state = 0x4e55580000000001ULL;
        for (;;) __asm__ volatile("wfe");
    }

    /* First invariant: bootloader memory data is present before VM takeover. */
    nux_boot_state = 0x4e55580000000002ULL;
    (void)boot_info;

    /* Interrupts remain masked until the kernel installs its own vectors. */
    for (;;) {
#if defined(__aarch64__)
        __asm__ volatile("wfe");
#else
        __asm__ volatile("hlt");
#endif
    }
}
