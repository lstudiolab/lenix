#ifndef LENIX_BOOT_PROTOCOL_H
#define LENIX_BOOT_PROTOCOL_H

#include <nux/types.h>

/*
 * First-party Lenix -> XCU/Nux handoff ABI.
 *
 * This is an owned Lenix contract, not a Limine protocol adapter. The boot
 * manager and kernel must be built against the same versioned definition.
 */
#define LENIX_BOOT_INFO_SIGNATURE 0x4c454e4958424f4fULL /* "LENIXBOO" */
#define LENIX_BOOT_INFO_VERSION 1u
#define LENIX_BOOT_MAX_MEMORY_REGIONS 512u
#define LENIX_BOOT_MAX_RESERVED_RANGES 128u

typedef enum lenix_memory_type {
    LENIX_MEMORY_USABLE = 1,
    LENIX_MEMORY_RESERVED,
    LENIX_MEMORY_FIRMWARE,
    LENIX_MEMORY_ACPI_RECLAIMABLE,
    LENIX_MEMORY_ACPI_NVS,
    LENIX_MEMORY_MMIO,
    LENIX_MEMORY_BOOTLOADER,
    LENIX_MEMORY_KERNEL,
    LENIX_MEMORY_MODULE
} lenix_memory_type;

typedef struct lenix_memory_region {
    nux_u64 physical_base;
    nux_u64 page_count;
    nux_u32 type;
    nux_u32 attributes;
} lenix_memory_region;

typedef struct lenix_reserved_range {
    nux_u64 physical_base;
    nux_u64 length;
    nux_u32 type;
    nux_u32 reserved;
} lenix_reserved_range;

typedef struct lenix_framebuffer_info {
    nux_u64 physical_base;
    nux_u64 byte_size;
    nux_u32 width;
    nux_u32 height;
    nux_u32 pixels_per_scanline;
    nux_u32 pixel_format;
} lenix_framebuffer_info;

typedef struct lenix_boot_info {
    nux_u64 signature;
    nux_u32 version;
    nux_u32 structure_size;
    nux_u32 architecture;
    nux_u32 flags;
    nux_u64 memory_map;
    nux_u32 memory_region_count;
    nux_u32 memory_region_size;
    nux_u64 reserved_ranges;
    nux_u32 reserved_range_count;
    nux_u32 reserved_range_size;
    nux_u64 firmware_root_table;
    nux_u64 smbios_table;
    nux_u64 framebuffer;
    nux_u64 boot_device_path;
    nux_u64 manifest_digest;
    nux_u64 kernel_physical_base;
    nux_u64 kernel_virtual_base;
    nux_u64 kernel_image_size;
    nux_u64 direct_map_base;
    nux_u64 direct_map_size;
} lenix_boot_info;

#define LENIX_BOOT_ARCH_X86_64 1u
#define LENIX_BOOT_ARCH_AARCH64 2u

#define LENIX_BOOT_FLAG_IMAGE_VERIFIED (1u << 0)
#define LENIX_BOOT_FLAG_SECURE_BOOT_ENABLED (1u << 1)
#define LENIX_BOOT_FLAG_FRAMEBUFFER_VALID (1u << 2)
#define LENIX_BOOT_FLAG_ACPI_VALID (1u << 3)
#define LENIX_BOOT_FLAG_SMBIOS_VALID (1u << 4)

#endif
