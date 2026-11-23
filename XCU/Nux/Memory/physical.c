#include <nux/memory.h>
#include "entry.h"

#define NUX_MEMMAP_USABLE 0ULL
#define NUX_MEMORY_MAX_REGIONS 256u
#define NUX_MEMORY_MAX_PAGES 16777216u

typedef struct nux_memmap_entry {
    nux_u64 base;
    nux_u64 length;
    nux_u64 type;
} nux_memmap_entry;

typedef struct nux_memory_region {
    nux_u64 start;
    nux_u64 next;
    nux_u64 end;
    nux_u32 page_index;
    nux_u32 page_count;
} nux_memory_region;

typedef struct nux_free_page {
    struct nux_free_page *next;
} nux_free_page;

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

static nux_memory_region regions[NUX_MEMORY_MAX_REGIONS];
static nux_u32 region_count;
static nux_u32 active_region;
static nux_u32 total_pages;
static nux_u8 page_bitmap[NUX_MEMORY_MAX_PAGES / 8u];
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

static nux_u32 nux_bitmap_test(nux_u32 index)
{
    return (page_bitmap[index >> 3] >> (index & 7u)) & 1u;
}

static void nux_bitmap_set(nux_u32 index)
{
    page_bitmap[index >> 3] |= (nux_u8)(1u << (index & 7u));
}

static void nux_bitmap_clear(nux_u32 index)
{
    page_bitmap[index >> 3] &= (nux_u8)~(1u << (index & 7u));
}

static nux_status nux_memory_page_index(nux_u64 physical, nux_u32 *out)
{
    if (!out || (physical & NUX_PAGE_MASK)) return NUX_E_INVALID;
    for (nux_u32 i = 0; i < region_count; ++i) {
        nux_memory_region *region = &regions[i];
        if (physical < region->start || physical >= region->next)
            continue;
        nux_u64 offset = (physical - region->start) >> NUX_PAGE_SHIFT;
        if (offset >= region->page_count) return NUX_E_ACCESS;
        *out = region->page_index + (nux_u32)offset;
        return NUX_OK;
    }
    return NUX_E_ACCESS;
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
    total_pages = 0;
    free_pages = NUX_NULL;
    memory_lock = 0;
    for (nux_u32 i = 0; i < sizeof(page_bitmap); ++i)
        page_bitmap[i] = 0;

    for (nux_u64 i = 0; i < count; ++i) {
        nux_memmap_entry *entry = entries[i];
        nux_u64 end;
        nux_u64 start;
        nux_u64 pages;

        if (entry == NUX_NULL || entry->type != NUX_MEMMAP_USABLE ||
            entry->length == 0 || entry->base > NUX_U64_MAX - entry->length)
            continue;

        end = nux_align_down(entry->base + entry->length);
        start = nux_align_up(entry->base);
        if (start == 0 && entry->base != 0)
            continue;
        if (start == 0) start = NUX_PAGE_SIZE;
        if (end <= start)
            continue;

        pages = (end - start) >> NUX_PAGE_SHIFT;
        if (pages > NUX_MEMORY_MAX_PAGES - total_pages)
            return NUX_E_LIMIT;
        if (region_count == NUX_MEMORY_MAX_REGIONS)
            return NUX_E_LIMIT;

        nux_memory_region *region = &regions[region_count++];
        region->start = start;
        region->next = start;
        region->end = end;
        region->page_index = total_pages;
        region->page_count = (nux_u32)pages;
        total_pages += (nux_u32)pages;
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
        nux_u32 index;
        if (nux_memory_page_index(result, &index) == NUX_OK &&
            !nux_bitmap_test(index)) {
            nux_bitmap_set(index);
        } else {
            result = 0;
        }
    } else {
        while (active_region < region_count) {
            nux_memory_region *region = &regions[active_region];
            if (region->next <= region->end &&
                region->end - region->next >= NUX_PAGE_SIZE) {
                result = region->next;
                region->next += NUX_PAGE_SIZE;
                nux_u32 index = region->page_index +
                    (nux_u32)((result - region->start) >> NUX_PAGE_SHIFT);
                if (index >= total_pages || nux_bitmap_test(index)) {
                    result = 0;
                    continue;
                }
                nux_bitmap_set(index);
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
    nux_u32 index;

    if (physical_address == 0 || (physical_address & NUX_PAGE_MASK) != 0)
        return NUX_E_INVALID;
    page = (nux_free_page *)nux_phys_to_virt(physical_address);
    if (page == NUX_NULL) return NUX_E_INVALID;

    nux_arch_lock(&memory_lock);
    if (nux_memory_page_index(physical_address, &index) != NUX_OK ||
        !nux_bitmap_test(index)) {
        nux_arch_unlock(&memory_lock);
        return NUX_E_ACCESS;
    }

    nux_bitmap_clear(index);
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
