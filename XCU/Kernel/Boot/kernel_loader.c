#include "uefi.h"

#define NUX_KERNEL_PATH \
    { '\\', 'E','F','I','\\','L','E','N','I','X','\\','N','U','X','.','E','L','F',0 }
#define NUX_KERNEL_VIRTUAL_BASE 0xffffffff80000000ULL
#define NUX_DIRECT_MAP_BASE 0xffff800000000000ULL
#define NUX_PHYSICAL_LIMIT 0x100000000ULL
#define NUX_KERNEL_MAP_LIMIT (64ULL * 1024ULL * 1024ULL)
#define NUX_BOOT_STACK_PAGES 16u
#define NUX_PAGE_SIZE 4096ULL
#define NUX_LARGE_PAGE_SIZE (2ULL * 1024ULL * 1024ULL)
#define NUX_PAGE_MASK (NUX_PAGE_SIZE - 1ULL)
#define NUX_LARGE_PAGE_MASK (NUX_LARGE_PAGE_SIZE - 1ULL)
#define NUX_ELF_PT_LOAD 1u
#define NUX_ELF_PF_X 1u
#define NUX_EFI_ERROR (EFI_ERROR_BIT | 1ULL)
#define NUX_MAX_MEMORY_REGIONS 256u
#define NUX_MAX_ELF_PROGRAM_HEADERS 128u

typedef struct __attribute__((packed)) nux_elf64_header {
    efi_u8 ident[16];
    efi_u16 type;
    efi_u16 machine;
    efi_u32 version;
    efi_u64 entry;
    efi_u64 program_offset;
    efi_u64 section_offset;
    efi_u32 flags;
    efi_u16 header_size;
    efi_u16 program_entry_size;
    efi_u16 program_count;
    efi_u16 section_entry_size;
    efi_u16 section_count;
    efi_u16 section_names;
} nux_elf64_header;

typedef struct __attribute__((packed)) nux_elf64_program {
    efi_u32 type;
    efi_u32 flags;
    efi_u64 offset;
    efi_u64 virtual_address;
    efi_u64 physical_address;
    efi_u64 file_size;
    efi_u64 memory_size;
    efi_u64 alignment;
} nux_elf64_program;

typedef struct nux_memmap_entry {
    efi_u64 base;
    efi_u64 length;
    efi_u64 type;
} nux_memmap_entry;

typedef struct nux_boot_info {
    efi_u64 memory_map;
    efi_u64 memory_map_count;
    efi_u64 hhdm_offset;
    efi_u64 firmware_table;
    efi_u64 flags;
} nux_boot_info;

static const efi_guid loaded_image_guid = {
    0x5b1b31a1, 0x9562, 0x11d2, {0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}
};
static const efi_guid simple_file_system_guid = {
    0x964e5b22, 0x6459, 0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}
};
static const efi_guid acpi20_guid = {
    0x8868e871, 0xe4f1, 0x11d3, {0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}
};
static const efi_char16 kernel_path[] = NUX_KERNEL_PATH;
static const efi_char16 loading[] = {
    'L','e','n','i','x',' ','K','e','r','n','e','l',' ','L','o','a','d','e','r','\r','\n',0
};
static const efi_char16 invalid_elf[] = {
    'L','e','n','i','x',':',' ','i','n','v','a','l','i','d',' ','N','u','x',' ','E','L','F',' ','i','m','a','g','e','\r','\n',0
};
static const efi_char16 failed[] = {
    'L','e','n','i','x',' ','K','e','r','n','e','l',' ','L','o','a','d','e','r',':',' ','f','a','i','l','e','d','\r','\n',0
};

static nux_boot_info boot_info;
static efi_u64 boot_stack_physical;
static nux_memmap_entry memory_regions[NUX_MAX_MEMORY_REGIONS];
static nux_memmap_entry *memory_region_pointers[NUX_MAX_MEMORY_REGIONS];

#if defined(__x86_64__)
static efi_u64 x86_pml4[512] __attribute__((aligned(4096)));
static efi_u64 x86_pdpt_low[512] __attribute__((aligned(4096)));
static efi_u64 x86_pdpt_direct[512] __attribute__((aligned(4096)));
static efi_u64 x86_pdpt_kernel[512] __attribute__((aligned(4096)));
static efi_u64 x86_pd_low[4][512] __attribute__((aligned(4096)));
static efi_u64 x86_pd_direct[4][512] __attribute__((aligned(4096)));
static efi_u64 x86_pd_kernel[512] __attribute__((aligned(4096)));
#elif defined(__aarch64__)
static efi_u64 arm_ttbr0[512] __attribute__((aligned(4096)));
static efi_u64 arm_ttbr1[512] __attribute__((aligned(4096)));
static efi_u64 arm_l1_identity[512] __attribute__((aligned(4096)));
static efi_u64 arm_l1_direct[512] __attribute__((aligned(4096)));
static efi_u64 arm_l1_kernel[512] __attribute__((aligned(4096)));
static efi_u64 arm_l2_identity[4][512] __attribute__((aligned(4096)));
static efi_u64 arm_l2_direct[4][512] __attribute__((aligned(4096)));
static efi_u64 arm_l2_kernel[512] __attribute__((aligned(4096)));
#endif

static void zero_memory(void *destination, efi_u64 size)
{
    efi_u8 *out = (efi_u8 *)destination;
    for (efi_u64 i = 0; i < size; ++i) out[i] = 0;
}

static void copy_memory(void *destination, const void *source, efi_u64 size)
{
    efi_u8 *out = (efi_u8 *)destination;
    const efi_u8 *in = (const efi_u8 *)source;
    for (efi_u64 i = 0; i < size; ++i) out[i] = in[i];
}

static efi_u64 align_up(efi_u64 value, efi_u64 alignment)
{
    if (value > ~0ULL - (alignment - 1ULL)) return 0;
    return (value + alignment - 1ULL) & ~(alignment - 1ULL);
}

#if defined(LENIX_QEMU_BOOT_TRACE)
static void debug_write(const char *message)
{
#if defined(__x86_64__)
    while (*message) {
        unsigned char value = (unsigned char)*message++;
        __asm__ volatile("outb %0, $0xe9" : : "a"(value));
    }
#elif defined(__aarch64__)
    register efi_u64 operation __asm__("x0") = 0x04;
    register const char *text __asm__("x1") = message;
    __asm__ volatile("hlt #0xf000" : "+r"(operation) : "r"(text) : "memory");
#endif
}
#endif

static void print(efi_system_table *system, const efi_char16 *message)
{
    if (system && system->console_out && system->console_out->output_string)
        (void)system->console_out->output_string(
            system->console_out, (efi_char16 *)message);
#if defined(LENIX_QEMU_BOOT_TRACE)
    char ascii[128];
    efi_uintn i = 0;
    while (message[i] && i + 1 < sizeof(ascii)) {
        ascii[i] = message[i] < 0x80 ? (char)message[i] : '?';
        ++i;
    }
    ascii[i] = 0;
    debug_write(ascii);
#endif
}

static efi_status fail(efi_system_table *system, efi_status status)
{
    print(system, failed);
    return status ? status : NUX_EFI_ERROR;
}

static int address_below_limit(efi_u64 address, efi_u64 length)
{
    return address < NUX_PHYSICAL_LIMIT &&
           length <= NUX_PHYSICAL_LIMIT - address;
}

static efi_status read_kernel(efi_handle image, efi_system_table *system,
                              void **buffer_out, efi_u64 *size_out)
{
    efi_loaded_image *loaded = (efi_loaded_image *)0;
    efi_loaded_image *parent = (efi_loaded_image *)0;
    efi_handle device_handle = (efi_handle)0;
    efi_simple_file_system *filesystem = (efi_simple_file_system *)0;
    efi_file_protocol *root = (efi_file_protocol *)0;
    efi_file_protocol *file = (efi_file_protocol *)0;
    efi_status status;
    efi_u64 size = 0;
    efi_uintn read_size;
    void *buffer = (void *)0;

    status = system->boot_services->handle_protocol(
        image, &loaded_image_guid, (void **)&loaded);
    if (EFI_ERROR(status) || !loaded) return status ? status : NUX_EFI_ERROR;
    if (!address_below_limit((efi_u64)(efi_uintn)loaded->image_base,
                             loaded->image_size))
        return NUX_EFI_ERROR;

    device_handle = loaded->device_handle;
    if (!device_handle && loaded->parent_handle) {
        status = system->boot_services->handle_protocol(
            loaded->parent_handle, &loaded_image_guid, (void **)&parent);
        if (!EFI_ERROR(status) && parent) device_handle = parent->device_handle;
    }
    if (!device_handle) return NUX_EFI_ERROR;

    status = system->boot_services->handle_protocol(
        device_handle, &simple_file_system_guid, (void **)&filesystem);
    if (EFI_ERROR(status) || !filesystem)
        return status ? status : NUX_EFI_ERROR;
    status = filesystem->open_volume(filesystem, &root);
    if (EFI_ERROR(status) || !root) return status ? status : NUX_EFI_ERROR;

    status = root->open(root, &file, (efi_char16 *)kernel_path, EFI_OPEN_READ, 0);
    if (EFI_ERROR(status) || !file) {
        (void)root->close(root);
        return status ? status : NUX_EFI_ERROR;
    }
    status = file->set_position(file, ~0ULL);
    if (!EFI_ERROR(status)) status = file->get_position(file, &size);
    if (EFI_ERROR(status) || size < sizeof(nux_elf64_header) ||
        size > EFI_MAX_IMAGE_SIZE) {
        (void)file->close(file);
        (void)root->close(root);
        return status ? status : NUX_EFI_ERROR;
    }
    status = system->boot_services->allocate_pool(
        EFI_ALLOCATE_LOADER_DATA, size, &buffer);
    if (EFI_ERROR(status) || !buffer) {
        (void)file->close(file);
        (void)root->close(root);
        return status ? status : NUX_EFI_ERROR;
    }
    status = file->set_position(file, 0);
    read_size = size;
    if (!EFI_ERROR(status)) status = file->read(file, &read_size, buffer);
    (void)file->close(file);
    (void)root->close(root);
    if (EFI_ERROR(status) || read_size != size) {
        (void)system->boot_services->free_pool(buffer);
        return status ? status : NUX_EFI_ERROR;
    }
    *buffer_out = buffer;
    *size_out = size;
    return EFI_SUCCESS;
}

static int validate_elf(const void *buffer, efi_u64 size,
                        efi_u64 *span_out, efi_u64 *entry_out)
{
    const nux_elf64_header *header = (const nux_elf64_header *)buffer;
    const nux_elf64_program *programs;
    efi_u64 minimum = ~0ULL;
    efi_u64 maximum = 0;
    int entry_executable = 0;

    if (!buffer || size < sizeof(*header) ||
        header->ident[0] != 0x7f || header->ident[1] != 'E' ||
        header->ident[2] != 'L' || header->ident[3] != 'F' ||
        header->ident[4] != 2 || header->ident[5] != 1 ||
        header->ident[6] != 1 || header->type != 2 ||
        header->version != 1 || header->header_size != sizeof(*header) ||
        header->program_entry_size != sizeof(nux_elf64_program) ||
        header->program_count == 0 ||
        header->program_count > NUX_MAX_ELF_PROGRAM_HEADERS)
        return 0;
#if defined(__x86_64__)
    if (header->machine != 62) return 0;
#elif defined(__aarch64__)
    if (header->machine != 183) return 0;
#endif
    if (header->program_offset > size ||
        (efi_u64)header->program_count * header->program_entry_size >
            size - header->program_offset)
        return 0;

    programs = (const nux_elf64_program *)((const efi_u8 *)buffer + header->program_offset);
    for (efi_u16 i = 0; i < header->program_count; ++i) {
        const nux_elf64_program *program = &programs[i];
        efi_u64 end;
        if (program->type != NUX_ELF_PT_LOAD) continue;
        if (program->memory_size == 0 || program->file_size > program->memory_size ||
            program->offset > size || program->file_size > size - program->offset ||
            program->virtual_address < NUX_KERNEL_VIRTUAL_BASE ||
            program->virtual_address > ~0ULL - program->memory_size)
            return 0;
        end = program->virtual_address + program->memory_size;
        if (program->virtual_address < minimum) minimum = program->virtual_address;
        if (end > maximum) maximum = end;
        if ((program->flags & NUX_ELF_PF_X) &&
            header->entry >= program->virtual_address && header->entry < end)
            entry_executable = 1;
        if (program->alignment && (program->alignment & (program->alignment - 1ULL)))
            return 0;
    }
    if (!entry_executable || minimum == ~0ULL || maximum <= minimum ||
        (minimum & NUX_PAGE_MASK) != 0)
        return 0;
    if ((minimum & ~NUX_LARGE_PAGE_MASK) != NUX_KERNEL_VIRTUAL_BASE)
        return 0;
    efi_u64 span = align_up(maximum - NUX_KERNEL_VIRTUAL_BASE, NUX_LARGE_PAGE_SIZE);
    if (!span || span > NUX_KERNEL_MAP_LIMIT || header->entry < NUX_KERNEL_VIRTUAL_BASE ||
        header->entry >= NUX_KERNEL_VIRTUAL_BASE + span)
        return 0;
    *span_out = span;
    *entry_out = header->entry;
    return 1;
}

static efi_status allocate_kernel(efi_system_table *system, const void *buffer,
                                  efi_u64 file_size, efi_u64 span,
                                  efi_u64 *physical_out)
{
    const nux_elf64_header *header = (const nux_elf64_header *)buffer;
    const nux_elf64_program *programs =
        (const nux_elf64_program *)((const efi_u8 *)buffer + header->program_offset);
    efi_u64 pages = (span >> 12) + (NUX_LARGE_PAGE_SIZE >> 12);
    efi_u64 allocation_base = 0xffffffffULL;
    efi_u64 physical;
    efi_status status;

    status = system->boot_services->allocate_pages(
        EFI_ALLOCATE_MAX_ADDRESS, EFI_LOADER_DATA, pages, &allocation_base);
    if (EFI_ERROR(status)) return status;
    physical = align_up(allocation_base, NUX_LARGE_PAGE_SIZE);
    if (!physical || !address_below_limit(allocation_base, pages << 12) ||
        !address_below_limit(physical, span))
        return NUX_EFI_ERROR;

    zero_memory((void *)(efi_uintn)physical, span);
    for (efi_u16 i = 0; i < header->program_count; ++i) {
        const nux_elf64_program *program = &programs[i];
        efi_u64 offset;
        if (program->type != NUX_ELF_PT_LOAD || program->file_size == 0) continue;
        offset = program->virtual_address - NUX_KERNEL_VIRTUAL_BASE;
        if (offset > span || program->file_size > span - offset ||
            program->offset > file_size || program->file_size > file_size - program->offset)
            return NUX_EFI_ERROR;
        copy_memory((void *)(efi_uintn)(physical + offset),
                    (const efi_u8 *)buffer + program->offset, program->file_size);
    }
    *physical_out = physical;
    return EFI_SUCCESS;
}

static efi_u64 find_firmware_root(efi_system_table *system)
{
    typedef struct { efi_guid guid; void *table; } efi_configuration_entry;
    efi_configuration_entry *entries =
        (efi_configuration_entry *)system->configuration_table;
    for (efi_uintn i = 0; i < system->number_of_table_entries; ++i) {
        const efi_guid *a = &entries[i].guid;
        if (a->data1 == acpi20_guid.data1 && a->data2 == acpi20_guid.data2 &&
            a->data3 == acpi20_guid.data3) {
            int equal = 1;
            for (efi_u32 j = 0; j < 8; ++j)
                if (a->data4[j] != acpi20_guid.data4[j]) equal = 0;
            if (equal) return (efi_u64)(efi_uintn)entries[i].table;
        }
    }
    return 0;
}

static int capture_memory_map(efi_system_table *system, void *map_buffer,
                              efi_uintn *map_key)
{
    efi_uintn map_size = EFI_MEMORY_MAP_BUFFER_SIZE;
    efi_uintn descriptor_size = 0;
    efi_u32 descriptor_version = 0;
    efi_status status = system->boot_services->get_memory_map(
        &map_size, map_buffer, map_key, &descriptor_size, &descriptor_version);
    if (EFI_ERROR(status) || descriptor_size < sizeof(efi_memory_descriptor) ||
        descriptor_size > 512 || map_size % descriptor_size)
        return 0;

    efi_uintn count = map_size / descriptor_size;
    efi_uintn stored = 0;
    for (efi_uintn i = 0; i < count; ++i) {
        const efi_memory_descriptor *descriptor =
            (const efi_memory_descriptor *)((const efi_u8 *)map_buffer +
                                             i * descriptor_size);
        efi_u64 base = descriptor->physical_start;
        if (!descriptor->number_of_pages ||
            descriptor->number_of_pages > (~0ULL >> 12) ||
            base >= NUX_PHYSICAL_LIMIT)
            continue;
        efi_u64 length = descriptor->number_of_pages << 12;
        if (length > NUX_PHYSICAL_LIMIT - base)
            length = NUX_PHYSICAL_LIMIT - base;
        if (!length) continue;
        if (stored == NUX_MAX_MEMORY_REGIONS) return 0;
        memory_regions[stored].base = base;
        memory_regions[stored].length = length;
        memory_regions[stored].type =
            descriptor->type == EFI_CONVENTIONAL_MEMORY ? 0ULL : 1ULL;
        memory_region_pointers[stored] = &memory_regions[stored];
        ++stored;
    }
    if (!stored) return 0;
    boot_info.memory_map = (efi_u64)(efi_uintn)memory_region_pointers;
    boot_info.memory_map_count = stored;
    boot_info.hhdm_offset = NUX_DIRECT_MAP_BASE;
    boot_info.firmware_table = find_firmware_root(system);
    if (boot_info.firmware_table >= NUX_PHYSICAL_LIMIT) boot_info.firmware_table = 0;
    boot_info.flags = 0;
    return 1;
}

#if defined(__x86_64__)
static int build_page_tables(efi_u64 kernel_physical, efi_u64 kernel_span)
{
    zero_memory(x86_pml4, sizeof(x86_pml4));
    zero_memory(x86_pdpt_low, sizeof(x86_pdpt_low));
    zero_memory(x86_pdpt_direct, sizeof(x86_pdpt_direct));
    zero_memory(x86_pdpt_kernel, sizeof(x86_pdpt_kernel));
    zero_memory(x86_pd_low, sizeof(x86_pd_low));
    zero_memory(x86_pd_direct, sizeof(x86_pd_direct));
    zero_memory(x86_pd_kernel, sizeof(x86_pd_kernel));
    if (!address_below_limit((efi_u64)(efi_uintn)x86_pml4, sizeof(x86_pml4)) ||
        !address_below_limit((efi_u64)(efi_uintn)x86_pdpt_low, sizeof(x86_pdpt_low)) ||
        !address_below_limit((efi_u64)(efi_uintn)x86_pdpt_direct, sizeof(x86_pdpt_direct)) ||
        !address_below_limit((efi_u64)(efi_uintn)x86_pdpt_kernel, sizeof(x86_pdpt_kernel)))
        return 0;

    x86_pml4[0] = (efi_u64)(efi_uintn)x86_pdpt_low | 3ULL;
    x86_pml4[256] = (efi_u64)(efi_uintn)x86_pdpt_direct | 3ULL;
    x86_pml4[511] = (efi_u64)(efi_uintn)x86_pdpt_kernel | 3ULL;
    for (efi_u32 directory = 0; directory < 4; ++directory) {
        x86_pdpt_low[directory] = (efi_u64)(efi_uintn)x86_pd_low[directory] | 3ULL;
        x86_pdpt_direct[directory] = (efi_u64)(efi_uintn)x86_pd_direct[directory] | 3ULL;
        for (efi_u32 page = 0; page < 512; ++page) {
            efi_u64 physical = ((efi_u64)directory * 512ULL + page) * NUX_LARGE_PAGE_SIZE;
            x86_pd_low[directory][page] = physical | 0x83ULL;
            x86_pd_direct[directory][page] = physical | 0x83ULL;
        }
    }
    x86_pdpt_kernel[510] = (efi_u64)(efi_uintn)x86_pd_kernel | 3ULL;
    for (efi_u32 page = 0; page < kernel_span / NUX_LARGE_PAGE_SIZE; ++page)
        x86_pd_kernel[page] = kernel_physical +
            (efi_u64)page * NUX_LARGE_PAGE_SIZE | 0x83ULL;
    return 1;
}

static __attribute__((noreturn)) void enter_kernel(efi_u64 entry, efi_u64 stack_top)
{
    register const nux_boot_info *argument __asm__("rdi") = &boot_info;
    register efi_u64 target __asm__("rax") = entry;
    register efi_u64 stack __asm__("rsi") = stack_top;
    efi_u64 root = (efi_u64)(efi_uintn)x86_pml4;
    efi_u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 4) | (1ULL << 5);
    __asm__ volatile("mov %0, %%cr4\nmov %1, %%cr3" : : "r"(cr4), "r"(root) : "memory");
    __asm__ volatile("mov %1, %%rsp\nandq $-16, %%rsp\njmp *%%rax" : : "D"(argument), "S"(stack), "a"(target) : "memory");
    __builtin_unreachable();
}
#elif defined(__aarch64__)
static int build_page_tables(efi_u64 kernel_physical, efi_u64 kernel_span)
{
    zero_memory(arm_ttbr0, sizeof(arm_ttbr0));
    zero_memory(arm_ttbr1, sizeof(arm_ttbr1));
    zero_memory(arm_l1_identity, sizeof(arm_l1_identity));
    zero_memory(arm_l1_direct, sizeof(arm_l1_direct));
    zero_memory(arm_l1_kernel, sizeof(arm_l1_kernel));
    zero_memory(arm_l2_identity, sizeof(arm_l2_identity));
    zero_memory(arm_l2_direct, sizeof(arm_l2_direct));
    zero_memory(arm_l2_kernel, sizeof(arm_l2_kernel));

    if (!address_below_limit((efi_u64)(efi_uintn)arm_ttbr0, sizeof(arm_ttbr0)) ||
        !address_below_limit((efi_u64)(efi_uintn)arm_ttbr1, sizeof(arm_ttbr1)) ||
        !address_below_limit((efi_u64)(efi_uintn)arm_l1_identity, sizeof(arm_l1_identity)) ||
        !address_below_limit((efi_u64)(efi_uintn)arm_l1_direct, sizeof(arm_l1_direct)) ||
        !address_below_limit((efi_u64)(efi_uintn)arm_l1_kernel, sizeof(arm_l1_kernel)))
        return 0;

    arm_ttbr0[0] = (efi_u64)(efi_uintn)arm_l1_identity | 3ULL;
    arm_ttbr1[256] = (efi_u64)(efi_uintn)arm_l1_direct | 3ULL;
    arm_ttbr1[511] = (efi_u64)(efi_uintn)arm_l1_kernel | 3ULL;
    for (efi_u32 directory = 0; directory < 4; ++directory) {
        arm_l1_identity[directory] = (efi_u64)(efi_uintn)arm_l2_identity[directory] | 3ULL;
        arm_l1_direct[directory] = (efi_u64)(efi_uintn)arm_l2_direct[directory] | 3ULL;
        for (efi_u32 page = 0; page < 512; ++page) {
            efi_u64 physical = ((efi_u64)directory * 512ULL + page) * NUX_LARGE_PAGE_SIZE;
            arm_l2_identity[directory][page] = physical | 0x701ULL;
            arm_l2_direct[directory][page] = physical | 0x701ULL;
        }
    }
    arm_l1_kernel[510] = (efi_u64)(efi_uintn)arm_l2_kernel | 3ULL;
    for (efi_u32 page = 0; page < kernel_span / NUX_LARGE_PAGE_SIZE; ++page)
        arm_l2_kernel[page] = kernel_physical +
            (efi_u64)page * NUX_LARGE_PAGE_SIZE | 0x701ULL;
    return 1;
}

static __attribute__((noreturn)) void enter_kernel(efi_u64 entry, efi_u64 stack_top)
{
    register const nux_boot_info *argument __asm__("x0") = &boot_info;
    register efi_u64 target __asm__("x16") = entry;
    register efi_u64 stack __asm__("x17") = stack_top;
    efi_u64 ttbr0 = (efi_u64)(efi_uintn)arm_ttbr0;
    efi_u64 ttbr1 = (efi_u64)(efi_uintn)arm_ttbr1;
    efi_u64 tcr = 16ULL | (1ULL << 8) | (1ULL << 10) | (3ULL << 12) |
                  (16ULL << 16) | (1ULL << 24) | (1ULL << 26) |
                  (3ULL << 28) | (2ULL << 30) | (2ULL << 32);
    efi_u64 mair = 0xffULL;
    efi_u64 sctlr;
    __asm__ volatile(
        "dsb sy\n"
        "msr mair_el1, %0\n"
        "msr tcr_el1, %1\n"
        "msr ttbr0_el1, %2\n"
        "msr ttbr1_el1, %3\n"
        "isb\n"
        "tlbi vmalle1\n"
        "dsb ish\n"
        "isb\n"
        "mrs %4, sctlr_el1\n"
        : "+r"(mair), "+r"(tcr), "+r"(ttbr0), "+r"(ttbr1), "=r"(sctlr)
        :
        : "memory");
    sctlr |= (1ULL << 0) | (1ULL << 2) | (1ULL << 12);
    __asm__ volatile(
        "msr sctlr_el1, %0\n"
        "dsb sy\n"
        "isb\n"
        "mov sp, %1\n"
        "mov x0, %2\n"
        "br %3\n"
        : : "r"(sctlr), "r"(stack), "r"(argument), "r"(target) : "memory");
    __builtin_unreachable();
}
#endif

efi_status nux_kernel_loader_main(efi_handle image, efi_system_table *system)
{
    void *kernel_buffer = (void *)0;
    void *memory_map_buffer = (void *)0;
    efi_u64 kernel_file_size = 0;
    efi_u64 kernel_span = 0;
    efi_u64 kernel_entry = 0;
    efi_u64 kernel_physical = 0;
    efi_uintn map_key = 0;
    efi_status status;

    if (!system || system->header.signature != EFI_SYSTEM_TABLE_SIGNATURE ||
        !system->boot_services)
        return EFI_ERROR_BIT;

    efi_u64 stack_pointer;
#if defined(__x86_64__)
    __asm__ volatile("mov %%rsp, %0" : "=r"(stack_pointer));
#elif defined(__aarch64__)
    __asm__ volatile("mov %0, sp" : "=r"(stack_pointer));
#endif
    if (!address_below_limit(stack_pointer, 1)) return fail(system, NUX_EFI_ERROR);

    print(system, loading);
    status = read_kernel(image, system, &kernel_buffer, &kernel_file_size);
    if (EFI_ERROR(status)) return fail(system, status);
    if (!validate_elf(kernel_buffer, kernel_file_size, &kernel_span, &kernel_entry)) {
        (void)system->boot_services->free_pool(kernel_buffer);
        print(system, invalid_elf);
        return fail(system, NUX_EFI_ERROR);
    }

    status = allocate_kernel(system, kernel_buffer, kernel_file_size,
                             kernel_span, &kernel_physical);
    (void)system->boot_services->free_pool(kernel_buffer);
    if (EFI_ERROR(status)) return fail(system, status);

    boot_stack_physical = 0xffffffffULL;
    status = system->boot_services->allocate_pages(
        EFI_ALLOCATE_MAX_ADDRESS, EFI_LOADER_DATA,
        NUX_BOOT_STACK_PAGES, &boot_stack_physical);
    if (EFI_ERROR(status) ||
        !address_below_limit(boot_stack_physical,
                             (efi_u64)NUX_BOOT_STACK_PAGES * NUX_PAGE_SIZE))
        return fail(system, status ? status : NUX_EFI_ERROR);
    zero_memory((void *)(efi_uintn)boot_stack_physical,
                (efi_u64)NUX_BOOT_STACK_PAGES * NUX_PAGE_SIZE);

    if (!build_page_tables(kernel_physical, kernel_span))
        return fail(system, NUX_EFI_ERROR);

    status = system->boot_services->allocate_pool(
        EFI_ALLOCATE_LOADER_DATA, EFI_MEMORY_MAP_BUFFER_SIZE, &memory_map_buffer);
    if (EFI_ERROR(status) || !memory_map_buffer)
        return fail(system, status ? status : NUX_EFI_ERROR);

    /* ExitBootServices may require a fresh map key; do not allocate after this. */
    int exited = 0;
    for (efi_u32 attempt = 0; attempt < 4; ++attempt) {
        if (!capture_memory_map(system, memory_map_buffer, &map_key))
            return fail(system, NUX_EFI_ERROR);
        status = system->boot_services->exit_boot_services(image, map_key);
        if (!EFI_ERROR(status)) {
            exited = 1;
            break;
        }
    }
    if (!exited) return status ? status : NUX_EFI_ERROR;
    enter_kernel(kernel_entry, NUX_DIRECT_MAP_BASE + boot_stack_physical +
        (efi_u64)NUX_BOOT_STACK_PAGES * NUX_PAGE_SIZE);
}
