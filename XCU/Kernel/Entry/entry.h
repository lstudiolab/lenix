#ifndef NUX_ENTRY_H
#define NUX_ENTRY_H

#include <nux/types.h>

typedef struct nux_boot_info {
    nux_u64 memory_map;
    nux_u64 memory_map_count;
    nux_u64 hhdm_offset;
    nux_u64 firmware_table;
    nux_u64 flags;
} nux_boot_info;

void nux_kernel_main(const nux_boot_info *boot_info) __attribute__((noreturn));
void nux_arch_early_init(void);

#endif
