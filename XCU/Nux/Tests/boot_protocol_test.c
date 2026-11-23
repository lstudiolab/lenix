#include <assert.h>
#include <stddef.h>
#include "boot_protocol.h"

_Static_assert(sizeof(lenix_memory_region) == 24, "memory region ABI changed");
_Static_assert(sizeof(lenix_reserved_range) == 24, "reserved range ABI changed");
_Static_assert(sizeof(lenix_framebuffer_info) == 32, "framebuffer ABI changed");
_Static_assert(sizeof(lenix_boot_info) == 136, "boot info ABI changed");
_Static_assert(offsetof(lenix_boot_info, memory_map) == 24, "memory map offset changed");
_Static_assert(offsetof(lenix_boot_info, firmware_root_table) == 56,
               "firmware table offset changed");
_Static_assert(offsetof(lenix_boot_info, kernel_physical_base) == 96,
               "kernel image offset changed");

int main(void)
{
    lenix_boot_info info = {0};
    lenix_memory_region region = {0};

    info.signature = LENIX_BOOT_INFO_SIGNATURE;
    info.version = LENIX_BOOT_INFO_VERSION;
    info.structure_size = sizeof(info);
    info.architecture = LENIX_BOOT_ARCH_X86_64;
    info.flags = LENIX_BOOT_FLAG_IMAGE_VERIFIED;
    info.memory_map = (nux_u64)(nux_uptr)&region;
    info.memory_region_count = 1;
    info.memory_region_size = sizeof(region);
    region.type = LENIX_MEMORY_USABLE;

    assert(info.signature == LENIX_BOOT_INFO_SIGNATURE);
    assert(info.version == 1);
    assert(info.structure_size == 136);
    assert(info.architecture == LENIX_BOOT_ARCH_X86_64);
    assert((info.flags & LENIX_BOOT_FLAG_IMAGE_VERIFIED) != 0);
    assert(info.memory_region_count == 1);
    assert(region.type == LENIX_MEMORY_USABLE);
    return 0;
}
