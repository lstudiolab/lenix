#include <nux/memory.h>
#include "entry.h"

typedef struct {
    nux_u64 base;
    nux_u64 length;
    nux_u64 type;
} test_memmap_entry;

static nux_u8 arena[5 * NUX_PAGE_SIZE] __attribute__((aligned(NUX_PAGE_SIZE)));

void nux_arch_lock(volatile nux_u32 *lock)
{
    while (*lock) { }
    *lock = 1;
}

void nux_arch_unlock(volatile nux_u32 *lock) { *lock = 0; }

int main(void)
{
    test_memmap_entry entry = { .base = 0, .length = 5 * NUX_PAGE_SIZE, .type = 0 };
    test_memmap_entry *entries[] = { &entry };
    nux_boot_info boot = {
        .memory_map = (nux_u64)(nux_uptr)entries,
        .memory_map_count = 1,
        .hhdm_offset = (nux_u64)(nux_uptr)arena
    };

    if (nux_memory_init(&boot) != NUX_OK) return 1;
    if (nux_phys_to_virt(0x1000) != arena + NUX_PAGE_SIZE) return 2;

    nux_u64 a = nux_phys_alloc_page();
    nux_u64 b = nux_phys_alloc_page();
    nux_u64 c = nux_phys_alloc_page();
    nux_u64 d = nux_phys_alloc_page();
    nux_u64 e = nux_phys_alloc_page();
    if (a != 0x1000 || b != 0x2000 || c != 0x3000 || d != 0x4000 || e != 0x5000) return 3;
    if (nux_phys_alloc_page() != 0) return 4;

    if (nux_phys_free_page(b) != NUX_OK) return 5;
    if (nux_phys_free_page(b) != NUX_E_ACCESS) return 6;
    if (nux_phys_alloc_page() != b) return 7;
    if (nux_phys_free_page(b) != NUX_OK) return 8;
    if (nux_phys_free_page(0x6000) != NUX_E_ACCESS) return 9;
    if (nux_phys_free_page(a) != NUX_OK) return 10;
    if (nux_phys_alloc_page() != a) return 11;
    return 0;
}
