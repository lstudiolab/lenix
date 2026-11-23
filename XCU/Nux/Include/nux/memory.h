#ifndef NUX_MEMORY_H
#define NUX_MEMORY_H

#include <nux/types.h>
#include <nux/status.h>

#define NUX_PAGE_SHIFT 12u
#define NUX_PAGE_SIZE  (1ULL << NUX_PAGE_SHIFT)
#define NUX_PAGE_MASK  (NUX_PAGE_SIZE - 1ULL)

struct nux_boot_info;

nux_status nux_memory_init(const struct nux_boot_info *boot_info);
nux_u64 nux_phys_alloc_page(void);
nux_status nux_phys_free_page(nux_u64 physical_address);
nux_status nux_phys_retain_page(nux_u64 physical_address);
nux_u32 nux_phys_page_refcount(nux_u64 physical_address);
void *nux_phys_to_virt(nux_u64 physical_address);

#endif
