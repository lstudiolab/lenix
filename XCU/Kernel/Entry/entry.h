#ifndef NUX_ENTRY_H
#define NUX_ENTRY_H

#include <nux/types.h>

#define NUX_BOOT_INFO_SIGNATURE 0x4e5558424f4f5431ULL
#define NUX_BOOT_INFO_VERSION 1u
#define NUX_BOOT_ARCH_X86_64 1u
#define NUX_BOOT_ARCH_AARCH64 2u
#define NUX_BOOT_FLAG_IMAGE_VALIDATED (1u << 0)

typedef struct nux_boot_info {
    nux_u64 memory_map;
    nux_u64 memory_map_count;
    nux_u64 hhdm_offset;
    nux_u64 firmware_table;
    nux_u64 flags;
    nux_u64 signature;
    nux_u32 version;
    nux_u32 structure_size;
    nux_u32 architecture;
    nux_u32 reserved;
    nux_u64 kernel_physical_base;
    nux_u64 kernel_virtual_base;
    nux_u64 kernel_image_size;
    nux_u64 direct_map_base;
    nux_u64 direct_map_size;
} nux_boot_info;

void nux_kernel_main(const nux_boot_info *boot_info) __attribute__((noreturn));
void nux_arch_early_init(void);

#endif
