#include <nux/memory.h>
#include "entry.h"

#define NUX_MEMMAP_USABLE 0ULL
#define NUX_MEMORY_MAX_REGIONS 256u

typedef struct nux_memmap_entry {
    nux_u64 base;
    nux_u64 length;
    nux_u64 type;
} nux_memmap_entry;

typedef struct nux_memory_region {
    nux_u64 start;
    nux_u64 next;
    nux_u64 end;
} nux_memory_region;

typedef struct nux_free_page {
    struct nux_free_page *next;
} nux_free_page;

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

static nux_memory_region regions[NUX_MEMORY_MAX_REGIONS];
static nux_u32 region_count;
static nux_u32 active_region;
static nux_u64 direct_map_offset;
static nux_free_page *free_pages;
static volatile nux_u32 memory_lock;

static nux_u64 nux_align_up(nux_u64 value)
{
    if (value > NUX_U64_MAX - NUX_PAGE_MASK)
        return 0;
    return (value + NUX_PAGE_MASK) & ~NUX_PAGE_MASK;
}

static nux_u64 nux_align_down(nux_u64 value)
{
    return value & ~NUX_PAGE_MASK;
}

nux_status nux_memory_init(const struct nux_boot_info *boot_info)
{
    nux_memmap_entry **entries;
    nux_u64 count;

    if (boot_info == NUX_NULL || boot_info->memory_map == 0 ||
        boot_info->memory_map_count == 0 || boot_info->hhdm_offset == 0)
        return NUX_E_INVALID;

    entries = (nux_memmap_entry **)(nux_uptr)boot_info->memory_map;
    count = boot_info->memory_map_count;
    direct_map_offset = boot_info->hhdm_offset;
    region_count = 0;
    active_region = 0;
    free_pages = NUX_NULL;
    memory_lock = 0;

    for (nux_u64 i = 0; i < count; ++i) {
        nux_memmap_entry *entry = entries[i];
        nux_u64 end;
        nux_u64 start;

        if (entry == NUX_NULL || entry->type != NUX_MEMMAP_USABLE ||
            entry->length == 0 || entry->base > NUX_U64_MAX - entry->length)
            continue;

        end = nux_align_down(entry->base + entry->length);
        start = nux_align_up(entry->base);
        if (start == 0 || end <= start)
            continue;

        if (region_count == NUX_MEMORY_MAX_REGIONS)
            return NUX_E_LIMIT;

        regions[region_count].start = start;
        regions[region_count].next = start;
        regions[region_count].end = end;
        ++region_count;
    }

    return region_count == 0 ? NUX_E_NO_MEMORY : NUX_OK;
}

nux_u64 nux_phys_alloc_page(void)
{
    nux_u64 result = 0;

    nux_arch_lock(&memory_lock);
    if (free_pages != NUX_NULL) {
        nux_free_page *page = free_pages;
        free_pages = page->next;
        result = (nux_u64)((nux_uptr)page - (nux_uptr)direct_map_offset);
    } else {
        while (active_region < region_count) {
            nux_memory_region *region = &regions[active_region];
            if (region->next <= region->end &&
                region->end - region->next >= NUX_PAGE_SIZE) {
                result = region->next;
                region->next += NUX_PAGE_SIZE;
                break;
            }
            ++active_region;
        }
    }
    nux_arch_unlock(&memory_lock);

    return result;
}

nux_status nux_phys_free_page(nux_u64 physical_address)
{
    nux_free_page *page;
    nux_u32 valid = 0;

    if (physical_address == 0 || (physical_address & NUX_PAGE_MASK) != 0)
        return NUX_E_INVALID;

    page = (nux_free_page *)nux_phys_to_virt(physical_address);
    if (page == NUX_NULL)
        return NUX_E_INVALID;

    nux_arch_lock(&memory_lock);
    for (nux_u32 i = 0; i < region_count; ++i) {
        if (physical_address >= regions[i].start &&
            physical_address < regions[i].next) {
            valid = 1;
            break;
        }
    }

    if (!valid) {
        nux_arch_unlock(&memory_lock);
        return NUX_E_ACCESS;
    }

    for (nux_free_page *item = free_pages; item != NUX_NULL; item = item->next) {
        if (item == page) {
            nux_arch_unlock(&memory_lock);
            return NUX_E_INVALID;
        }
    }

    page->next = free_pages;
    free_pages = page;
    nux_arch_unlock(&memory_lock);
    return NUX_OK;
}

void *nux_phys_to_virt(nux_u64 physical_address)
{
    if (direct_map_offset == 0 ||
        physical_address > NUX_U64_MAX - direct_map_offset)
        return NUX_NULL;
    return (void *)(nux_uptr)(physical_address + direct_map_offset);
}
