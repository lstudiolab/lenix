#include "uefi.h"
#include "entry.h"
#include <nux/boot.h>

#define LENIX_DIRECT_MAP 0xffff800000000000ULL
#define LENIX_KERNEL_BASE 0xffffffff80000000ULL
#define LENIX_DIRECT_LIMIT (64ULL * 1024ULL * 1024ULL * 1024ULL)
#define LENIX_KERNEL_MAP_LIMIT (128ULL * 1024ULL * 1024ULL)
#define LENIX_BOOT_STACK_PAGES 32u
#define LENIX_PAGE_SIZE 4096ULL
#define LENIX_PAGE_MASK (LENIX_PAGE_SIZE - 1ULL)
#define LENIX_MAX_REGIONS 256u
#define EFI_INVALID_PARAMETER (EFI_ERROR_BIT | 2ULL)
#define EFI_PE_SIGNATURE 0x00004550u
#define EFI_PE32_PLUS 0x020bu
#define EFI_MACHINE_X64 0x8664u
#define EFI_MACHINE_ARM64 0xaa64u
#define EFI_SECTION_EXECUTE 0x20000000u
#define EFI_SECTION_READ 0x40000000u
#define EFI_SECTION_WRITE 0x80000000u

typedef struct {
    efi_u64 base;
    efi_u64 length;
    efi_u64 type;
} nux_boot_memory_entry;

typedef struct {
    efi_guid guid;
    void *table;
} efi_configuration_table;

static const efi_guid loaded_image_guid = {
    0x5b1b31a1, 0x9562, 0x11d2, {0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}
};
static const efi_guid acpi20_guid = {
    0x8868e871, 0xe4f1, 0x11d3, {0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}
};
static const efi_guid acpi10_guid = {
    0xeb9d2d30, 0x2d88, 0x11d3, {0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}
};

static efi_u8 memory_map_buffer[EFI_MEMORY_MAP_BUFFER_SIZE] __attribute__((aligned(16)));
static nux_boot_memory_entry memory_entries[LENIX_MAX_REGIONS] __attribute__((aligned(16)));
static nux_boot_memory_entry *memory_entry_pointers[LENIX_MAX_REGIONS] __attribute__((aligned(16)));
static nux_boot_info kernel_boot_info __attribute__((aligned(16)));
static efi_u64 boot_stack_physical;

extern void nux_arch_early_init(void);
extern void nux_kernel_main(const nux_boot_info *boot_info) __attribute__((noreturn));
extern void nux_arch_enter_kernel(efi_u64 entry, efi_u64 boot_info,
                                  efi_u64 stack_top, efi_u64 early_init)
    __attribute__((noreturn));

static efi_u16 read_u16(const efi_u8 *p)
{
    return (efi_u16)p[0] | ((efi_u16)p[1] << 8);
}

static efi_u32 read_u32(const efi_u8 *p)
{
    return (efi_u32)p[0] | ((efi_u32)p[1] << 8) |
           ((efi_u32)p[2] << 16) | ((efi_u32)p[3] << 24);
}

static efi_u64 read_u64(const efi_u8 *p)
{
    return (efi_u64)read_u32(p) | ((efi_u64)read_u32(p + 4) << 32);
}

static efi_u64 align_down(efi_u64 value, efi_u64 alignment)
{
    return value & ~(alignment - 1ULL);
}

static efi_u64 align_up(efi_u64 value, efi_u64 alignment)
{
    if (value > ~0ULL - (alignment - 1ULL)) return 0;
    return (value + alignment - 1ULL) & ~(alignment - 1ULL);
}

static efi_u64 physical_address(const void *pointer)
{
    return (efi_u64)(efi_uintn)pointer;
}

static void *high_direct_map(efi_u64 physical)
{
    if (physical >= LENIX_DIRECT_LIMIT) return (void *)0;
    return (void *)(efi_uintn)(LENIX_DIRECT_MAP + physical);
}

static void *find_acpi_root(efi_system_table *system)
{
    efi_configuration_table *tables =
        (efi_configuration_table *)system->configuration_table;
    for (efi_uintn i = 0; i < system->number_of_table_entries; ++i) {
        if (tables[i].guid.data1 == acpi20_guid.data1 &&
            tables[i].guid.data2 == acpi20_guid.data2 &&
            tables[i].guid.data3 == acpi20_guid.data3)
            return tables[i].table;
    }
    for (efi_uintn i = 0; i < system->number_of_table_entries; ++i) {
        if (tables[i].guid.data1 == acpi10_guid.data1 &&
            tables[i].guid.data2 == acpi10_guid.data2 &&
            tables[i].guid.data3 == acpi10_guid.data3)
            return tables[i].table;
    }
    return (void *)0;
}

static efi_status collect_memory_map(efi_system_table *system,
                                     efi_handle image,
                                     efi_loaded_image *loaded)
{
    efi_boot_services_prefix *services = system->boot_services;
    efi_status status = services->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA, LENIX_BOOT_STACK_PAGES,
        &boot_stack_physical);
    if (EFI_ERROR(status)) return status;

#if defined(__aarch64__)
    /* UEFI may enter at EL2. Normalize to EL1 before installing TTBR1. */
    nux_arch_early_init();
#endif

    for (efi_u32 attempt = 0; attempt < 4; ++attempt) {
        efi_uintn map_size = sizeof(memory_map_buffer);
        efi_uintn map_key = 0;
        efi_uintn descriptor_size = 0;
        efi_u32 descriptor_version = 0;
        status = services->get_memory_map(&map_size, memory_map_buffer,
                                          &map_key, &descriptor_size,
                                          &descriptor_version);
        if (EFI_ERROR(status)) return status;
        if (descriptor_size < sizeof(efi_memory_descriptor) ||
            descriptor_size == 0 || map_size % descriptor_size != 0)
            return EFI_ERROR_BIT | 2ULL;

        efi_uintn descriptors = map_size / descriptor_size;
        efi_u32 count = 0;
        for (efi_uintn i = 0; i < descriptors; ++i) {
            efi_memory_descriptor *descriptor =
                (efi_memory_descriptor *)(memory_map_buffer + i * descriptor_size);
            if (descriptor->type != EFI_CONVENTIONAL_MEMORY ||
                descriptor->number_of_pages == 0 ||
                descriptor->physical_start >= LENIX_DIRECT_LIMIT)
                continue;
            if (count == LENIX_MAX_REGIONS) return EFI_ERROR_BIT | 3ULL;
            efi_u64 length = descriptor->number_of_pages * LENIX_PAGE_SIZE;
            if (length / LENIX_PAGE_SIZE != descriptor->number_of_pages)
                return EFI_ERROR_BIT | 4ULL;
            if (length > LENIX_DIRECT_LIMIT - descriptor->physical_start)
                length = LENIX_DIRECT_LIMIT - descriptor->physical_start;
            memory_entries[count].base = descriptor->physical_start;
            memory_entries[count].length = length;
            memory_entries[count].type = 0;
            memory_entry_pointers[count] =
                (nux_boot_memory_entry *)(efi_uintn)
                (LENIX_DIRECT_MAP + physical_address(&memory_entries[count]));
            ++count;
        }
        if (count == 0) return EFI_ERROR_BIT | 5ULL;

        kernel_boot_info.memory_map =
            LENIX_DIRECT_MAP + physical_address(memory_entry_pointers);
        kernel_boot_info.memory_map_count = count;
        kernel_boot_info.hhdm_offset = LENIX_DIRECT_MAP;
        kernel_boot_info.firmware_table = 0;
        void *acpi = find_acpi_root(system);
        if (acpi) kernel_boot_info.firmware_table =
            LENIX_DIRECT_MAP + physical_address(acpi);
        kernel_boot_info.flags = 0;

        status = services->exit_boot_services(image, map_key);
        if (status == EFI_INVALID_PARAMETER) continue;
        if (EFI_ERROR(status)) return status;
        (void)loaded;
        return EFI_SUCCESS;
    }
    return EFI_INVALID_PARAMETER;
}

typedef struct {
    efi_u64 image_base;
    efi_u64 image_size;
    efi_u64 headers_size;
    efi_u64 section_table;
    efi_u16 section_count;
    efi_u16 machine;
} nux_pe_image;

static efi_status parse_pe_image(efi_loaded_image *loaded, nux_pe_image *out)
{
    const efi_u8 *base = (const efi_u8 *)loaded->image_base;
    if (!base || loaded->image_size < 0x100 ||
        read_u16(base) != 0x5a4d)
        return EFI_ERROR_BIT | 6ULL;
    efi_u32 pe_offset = read_u32(base + 0x3c);
    if (pe_offset > loaded->image_size - 24 ||
        read_u32(base + pe_offset) != EFI_PE_SIGNATURE)
        return EFI_ERROR_BIT | 7ULL;
    const efi_u8 *coff = base + pe_offset + 4;
    efi_u16 machine = read_u16(coff);
    efi_u16 sections = read_u16(coff + 2);
    efi_u16 optional_size = read_u16(coff + 16);
    const efi_u8 *optional = coff + 20;
    if (optional_size < 64 || (efi_u64)(optional - base) + optional_size > loaded->image_size ||
        read_u16(optional) != EFI_PE32_PLUS)
        return EFI_ERROR_BIT | 8ULL;
#if defined(__x86_64__)
    if (machine != EFI_MACHINE_X64) return EFI_ERROR_BIT | 9ULL;
#elif defined(__aarch64__)
    if (machine != EFI_MACHINE_ARM64) return EFI_ERROR_BIT | 10ULL;
#endif
    efi_u64 image_size = read_u32(optional + 56);
    efi_u64 headers_size = read_u32(optional + 60);
    efi_u64 section_table = (efi_u64)(optional - base) + optional_size;
    if (image_size == 0 || image_size > loaded->image_size ||
        image_size > LENIX_KERNEL_MAP_LIMIT || headers_size > image_size ||
        section_table > loaded->image_size ||
        (efi_u64)sections * 40ULL > loaded->image_size - section_table)
        return EFI_ERROR_BIT | 11ULL;
    efi_u64 image_base = physical_address(loaded->image_base);
    if ((image_base & LENIX_PAGE_MASK) != 0 ||
        image_base >= LENIX_DIRECT_LIMIT ||
        image_size > LENIX_DIRECT_LIMIT - image_base)
        return EFI_ERROR_BIT | 12ULL;

    out->image_base = image_base;
    out->image_size = image_size;
    out->headers_size = headers_size;
    out->section_table = section_table;
    out->section_count = sections;
    out->machine = machine;
    return EFI_SUCCESS;
}

#if defined(__x86_64__)
#define X86_PRESENT 0x001ULL
#define X86_WRITE 0x002ULL
#define X86_LARGE 0x080ULL
#define X86_NX (1ULL << 63)
#define X86_ADDRESS 0x000ffffffffff000ULL
#define X86_LARGE_ADDRESS 0x000fffffffe00000ULL

static efi_u64 x86_root[512] __attribute__((aligned(4096)));
static efi_u64 x86_low_pdpt[512] __attribute__((aligned(4096)));
static efi_u64 x86_hhdm_pdpt[512] __attribute__((aligned(4096)));
static efi_u64 x86_low_pd[64][512] __attribute__((aligned(4096)));
static efi_u64 x86_hhdm_pd[64][512] __attribute__((aligned(4096)));
static efi_u64 x86_kernel_pdpt[512] __attribute__((aligned(4096)));
static efi_u64 x86_kernel_pd[512] __attribute__((aligned(4096)));
static efi_u64 x86_kernel_pt[64][512] __attribute__((aligned(4096)));

static efi_status build_kernel_mappings(const nux_pe_image *image)
{
    efi_u64 pages = align_up(image->image_size, LENIX_PAGE_SIZE) / LENIX_PAGE_SIZE;
    efi_u64 pt_count = (pages + 511ULL) / 512ULL;
    if (pt_count == 0 || pt_count > 64) return EFI_ERROR_BIT | 13ULL;

    for (efi_u32 i = 0; i < 64; ++i) {
        x86_low_pdpt[i] = physical_address(x86_low_pd[i]) | X86_PRESENT | X86_WRITE;
        x86_hhdm_pdpt[i] = physical_address(x86_hhdm_pd[i]) | X86_PRESENT | X86_WRITE;
        for (efi_u32 j = 0; j < 512; ++j) {
            efi_u64 pa = ((efi_u64)i * 512ULL + j) * 0x200000ULL;
            efi_u64 entry = pa | X86_PRESENT | X86_WRITE | X86_LARGE;
            x86_low_pd[i][j] = entry;
            x86_hhdm_pd[i][j] = entry;
        }
    }
    x86_root[0] = physical_address(x86_low_pdpt) | X86_PRESENT | X86_WRITE;
    x86_root[256] = physical_address(x86_hhdm_pdpt) | X86_PRESENT | X86_WRITE;
    x86_root[511] = physical_address(x86_kernel_pdpt) | X86_PRESENT | X86_WRITE;
    x86_kernel_pdpt[510] = physical_address(x86_kernel_pd) | X86_PRESENT | X86_WRITE;

    for (efi_u64 i = 0; i < pt_count; ++i)
        x86_kernel_pd[i] = physical_address(x86_kernel_pt[i]) |
                           X86_PRESENT | X86_WRITE;

    for (efi_u64 page = 0; page < pages; ++page) {
        efi_u64 pa = image->image_base + page * LENIX_PAGE_SIZE;
        x86_kernel_pt[page / 512ULL][page % 512ULL] =
            (pa & X86_ADDRESS) | X86_PRESENT | X86_NX;
    }

    const efi_u8 *base = (const efi_u8 *)(efi_uintn)image->image_base;
    efi_u64 header_pages = align_up(image->headers_size, LENIX_PAGE_SIZE) / LENIX_PAGE_SIZE;
    for (efi_u64 page = 0; page < header_pages; ++page)
        x86_kernel_pt[page / 512ULL][page % 512ULL] =
            ((image->image_base + page * LENIX_PAGE_SIZE) & X86_ADDRESS) |
            X86_PRESENT | X86_NX;

    for (efi_u32 i = 0; i < image->section_count; ++i) {
        const efi_u8 *section = base + image->section_table + (efi_u64)i * 40ULL;
        efi_u64 virtual_size = read_u32(section + 8);
        efi_u64 virtual_address = read_u32(section + 12);
        efi_u64 raw_size = read_u32(section + 16);
        efi_u64 size = virtual_size > raw_size ? virtual_size : raw_size;
        efi_u32 characteristics = read_u32(section + 36);
        if (!size) continue;
        if ((characteristics & EFI_SECTION_WRITE) &&
            (characteristics & EFI_SECTION_EXECUTE))
            return EFI_ERROR_BIT | 14ULL;
        if (virtual_address >= image->image_size ||
            size > image->image_size - virtual_address)
            return EFI_ERROR_BIT | 15ULL;
        efi_u64 first = virtual_address / LENIX_PAGE_SIZE;
        efi_u64 last = align_up(virtual_address + size, LENIX_PAGE_SIZE) /
                       LENIX_PAGE_SIZE;
        efi_u64 flags = X86_PRESENT;
        if (characteristics & EFI_SECTION_WRITE) flags |= X86_WRITE;
        if (!(characteristics & EFI_SECTION_EXECUTE)) flags |= X86_NX;
        for (efi_u64 page = first; page < last; ++page) {
            efi_u64 pa = image->image_base + page * LENIX_PAGE_SIZE;
            x86_kernel_pt[page / 512ULL][page % 512ULL] =
                (pa & X86_ADDRESS) | flags;
        }
    }
    return EFI_SUCCESS;
}

static void activate_kernel_mappings(void)
{
    efi_u32 low, high;
    efi_u64 cr0, root = physical_address(x86_root);
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0xc0000080u));
    low |= 1u << 11;
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(0xc0000080u));
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 1ULL << 16;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
    __asm__ volatile("mov %0, %%cr3" : : "r"(root) : "memory");
}

#elif defined(__aarch64__)
#define ARM_VALID 1ULL
#define ARM_TABLE 3ULL
#define ARM_AF (1ULL << 10)
#define ARM_SH_INNER (3ULL << 8)
#define ARM_AP_RO (2ULL << 6)
#define ARM_PXN (1ULL << 53)
#define ARM_UXN (1ULL << 54)
#define ARM_ADDRESS 0x0000fffffffff000ULL

static efi_u64 arm_root[512] __attribute__((aligned(4096)));
static efi_u64 arm_hhdm_l1[512] __attribute__((aligned(4096)));
static efi_u64 arm_kernel_l1[512] __attribute__((aligned(4096)));
static efi_u64 arm_kernel_l2[512] __attribute__((aligned(4096)));
static efi_u64 arm_kernel_pt[64][512] __attribute__((aligned(4096)));

static efi_status build_kernel_mappings(const nux_pe_image *image)
{
    efi_u64 pages = align_up(image->image_size, LENIX_PAGE_SIZE) / LENIX_PAGE_SIZE;
    efi_u64 pt_count = (pages + 511ULL) / 512ULL;
    if (pt_count == 0 || pt_count > 64) return EFI_ERROR_BIT | 13ULL;

    arm_root[256] = physical_address(arm_hhdm_l1) | ARM_TABLE;
    for (efi_u32 i = 0; i < 64; ++i) {
        efi_u64 pa = (efi_u64)i << 30;
        arm_hhdm_l1[i] = pa | ARM_VALID | ARM_AF | ARM_SH_INNER |
                         ARM_PXN | ARM_UXN;
    }
    arm_root[511] = physical_address(arm_kernel_l1) | ARM_TABLE;
    arm_kernel_l1[510] = physical_address(arm_kernel_l2) | ARM_TABLE;
    for (efi_u64 i = 0; i < pt_count; ++i)
        arm_kernel_l2[i] = physical_address(arm_kernel_pt[i]) | ARM_TABLE;

    for (efi_u64 page = 0; page < pages; ++page) {
        efi_u64 pa = image->image_base + page * LENIX_PAGE_SIZE;
        arm_kernel_pt[page / 512ULL][page % 512ULL] =
            (pa & ARM_ADDRESS) | ARM_VALID | ARM_TABLE | ARM_AF |
            ARM_SH_INNER | ARM_PXN | ARM_UXN;
    }

    const efi_u8 *base = (const efi_u8 *)(efi_uintn)image->image_base;
    efi_u64 header_pages = align_up(image->headers_size, LENIX_PAGE_SIZE) / LENIX_PAGE_SIZE;
    for (efi_u64 page = 0; page < header_pages; ++page)
        arm_kernel_pt[page / 512ULL][page % 512ULL] =
            ((image->image_base + page * LENIX_PAGE_SIZE) & ARM_ADDRESS) |
            ARM_VALID | ARM_TABLE | ARM_AF | ARM_SH_INNER |
            ARM_AP_RO | ARM_PXN | ARM_UXN;

    for (efi_u32 i = 0; i < image->section_count; ++i) {
        const efi_u8 *section = base + image->section_table + (efi_u64)i * 40ULL;
        efi_u64 virtual_size = read_u32(section + 8);
        efi_u64 virtual_address = read_u32(section + 12);
        efi_u64 raw_size = read_u32(section + 16);
        efi_u64 size = virtual_size > raw_size ? virtual_size : raw_size;
        efi_u32 characteristics = read_u32(section + 36);
        if (!size) continue;
        if ((characteristics & EFI_SECTION_WRITE) &&
            (characteristics & EFI_SECTION_EXECUTE))
            return EFI_ERROR_BIT | 14ULL;
        if (virtual_address >= image->image_size ||
            size > image->image_size - virtual_address)
            return EFI_ERROR_BIT | 15ULL;
        efi_u64 first = virtual_address / LENIX_PAGE_SIZE;
        efi_u64 last = align_up(virtual_address + size, LENIX_PAGE_SIZE) /
                       LENIX_PAGE_SIZE;
        efi_u64 flags = ARM_VALID | ARM_TABLE | ARM_AF | ARM_SH_INNER | ARM_UXN;
        if (characteristics & EFI_SECTION_WRITE) flags |= 0;
        else flags |= ARM_AP_RO;
        if (!(characteristics & EFI_SECTION_EXECUTE)) flags |= ARM_PXN;
        for (efi_u64 page = first; page < last; ++page) {
            efi_u64 pa = image->image_base + page * LENIX_PAGE_SIZE;
            arm_kernel_pt[page / 512ULL][page % 512ULL] =
                (pa & ARM_ADDRESS) | flags;
        }
    }
    return EFI_SUCCESS;
}

static void activate_kernel_mappings(void)
{
    efi_u64 tcr, mair = 0xffULL, root = physical_address(arm_root);
    __asm__ volatile("mrs %0, tcr_el1" : "=r"(tcr));
    tcr &= ~((0x3fULL << 16) | (3ULL << 24) | (3ULL << 26) |
             (3ULL << 28) | (3ULL << 30) | (7ULL << 32));
    tcr |= (16ULL << 16) | (1ULL << 24) | (1ULL << 26) |
           (3ULL << 28) | (2ULL << 30) | (5ULL << 32);
    __asm__ volatile(
        "msr mair_el1, %0; msr tcr_el1, %1; msr ttbr1_el1, %2; "
        "dsb ishst; tlbi vmalle1; dsb ish; isb"
        : : "r"(mair), "r"(tcr), "r"(root) : "memory");
}
#else
#error Unsupported Nux EFI architecture
#endif

efi_status nux_uefi_kernel_main(efi_handle image, efi_system_table *system)
{
    if (!system || system->header.signature != EFI_SYSTEM_TABLE_SIGNATURE ||
        !system->boot_services)
        return EFI_ERROR_BIT | 1ULL;

    efi_loaded_image *loaded = (efi_loaded_image *)0;
    efi_status status = system->boot_services->handle_protocol(
        image, &loaded_image_guid, (void **)&loaded);
    if (EFI_ERROR(status) || !loaded) return status ? status : EFI_ERROR_BIT;

    status = collect_memory_map(system, image, loaded);
    if (EFI_ERROR(status)) return status;

    nux_pe_image pe;
    status = parse_pe_image(loaded, &pe);
    if (EFI_ERROR(status)) nux_boot_fail(20);
    status = build_kernel_mappings(&pe);
    if (EFI_ERROR(status)) nux_boot_fail(21);

    activate_kernel_mappings();

    efi_u64 image_base = physical_address(loaded->image_base);
    efi_u64 kernel_entry = LENIX_KERNEL_BASE +
        ((efi_u64)(efi_uintn)nux_kernel_main - image_base);
    efi_u64 early_entry = LENIX_KERNEL_BASE +
        ((efi_u64)(efi_uintn)nux_arch_early_init - image_base);
    efi_u64 boot_info = LENIX_DIRECT_MAP + physical_address(&kernel_boot_info);
    efi_u64 stack_top = LENIX_DIRECT_MAP + boot_stack_physical +
        (efi_u64)LENIX_BOOT_STACK_PAGES * LENIX_PAGE_SIZE;

    nux_arch_enter_kernel(kernel_entry, boot_info, stack_top, early_entry);
}
