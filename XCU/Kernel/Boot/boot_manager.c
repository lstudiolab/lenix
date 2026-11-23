#include "uefi.h"

static const efi_guid loaded_image_guid = {
    0x5b1b31a1, 0x9562, 0x11d2, {0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}
};
static const efi_guid simple_file_system_guid = {
    0x964e5b22, 0x6459, 0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}
};
static const efi_char16 banner[] = {
    'L','e','n','i','x',' ','B','o','o','t',' ','M','a','n','a','g','e','r','\r','\n',0
};
static const efi_char16 loading[] = {
    'L','o','a','d','i','n','g',' ','N','u','x',' ','k','e','r','n','e','l',' ','i','m','a','g','e','\r','\n',0
};
static const efi_char16 failed[] = {
    'L','e','n','i','x',' ','B','o','o','t',' ','M','a','n','a','g','e','r',':',' ','b','o','o','t',' ','f','a','i','l','e','d','\r','\n',0
};
static const efi_char16 kernel_path[] = {
    '\\','E','F','I','\\','L','E','N','I','X','\\','N','U','X','.','E','F','I',0
};

static const efi_char16 invalid_image[] = {
    'L','e','n','i','x',':',' ','i','n','v','a','l','i','d',' ','E','F','I',' ',
    'k','e','r','n','e','l',' ','i','m','a','g','e',' ','(','P','E','/','C','O','F','F',' ',
    'r','e','q','u','i','r','e','d',')','\\r','\\n',0
};

static efi_u16 read_u16(const efi_u8 *bytes)
{
    return (efi_u16)((efi_u16)bytes[0] | ((efi_u16)bytes[1] << 8));
}

static efi_u32 read_u32(const efi_u8 *bytes)
{
    return (efi_u32)bytes[0] | ((efi_u32)bytes[1] << 8) |
           ((efi_u32)bytes[2] << 16) | ((efi_u32)bytes[3] << 24);
}

/* UEFI LoadImage accepts PE/COFF, not the ELF kernel emitted by XCU today. */
static int valid_kernel_image(const efi_u8 *image, efi_u64 size)
{
    efi_u32 pe_offset;
    efi_u16 machine;
    efi_u16 section_count;
    efi_u16 optional_size;
    efi_u16 optional_magic;
    efi_u32 image_size;
    efi_u32 headers_size;
    efi_u64 section_table;
    efi_u64 section_bytes;

    if (!image || size < 64 || image[0] != 'M' || image[1] != 'Z')
        return 0;
    pe_offset = read_u32(image + 0x3c);
    if ((efi_u64)pe_offset > size || size - pe_offset < 24)
        return 0;
    if (read_u32(image + pe_offset) != 0x00004550u)
        return 0;

    machine = read_u16(image + pe_offset + 4);
#if defined(__x86_64__)
    if (machine != 0x8664u) return 0;
#elif defined(__aarch64__)
    if (machine != 0xaa64u) return 0;
#else
#error Unsupported EFI boot architecture
#endif

    section_count = read_u16(image + pe_offset + 6);
    optional_size = read_u16(image + pe_offset + 20);
    if (section_count == 0 || section_count > 96 ||
        optional_size < 70 ||
        (efi_u64)pe_offset + 24u + optional_size > size)
        return 0;

    const efi_u8 *optional = image + pe_offset + 24;
    optional_magic = read_u16(optional);
    if (optional_magic != 0x20bu || read_u16(optional + 68) != 10u)
        return 0; /* PE32+ EFI application subsystem. */

    image_size = read_u32(optional + 56);
    headers_size = read_u32(optional + 60);
    if (image_size == 0 || image_size > EFI_MAX_IMAGE_SIZE ||
        headers_size == 0 || headers_size > size)
        return 0;

    section_table = (efi_u64)pe_offset + 24u + optional_size;
    section_bytes = (efi_u64)section_count * 40u;
    if (section_table > size || section_bytes > size - section_table ||
        section_table + section_bytes > headers_size)
        return 0;

    for (efi_u16 i = 0; i < section_count; ++i) {
        const efi_u8 *section = image + section_table + (efi_u64)i * 40u;
        efi_u32 virtual_size = read_u32(section + 8);
        efi_u32 virtual_address = read_u32(section + 12);
        efi_u32 raw_size = read_u32(section + 16);
        efi_u32 raw_offset = read_u32(section + 20);
        if (raw_size && ((efi_u64)raw_offset > size ||
                         (efi_u64)raw_size > size - raw_offset))
            return 0;
        if (virtual_address > image_size ||
            virtual_size > image_size - virtual_address ||
            raw_size > image_size - virtual_address)
            return 0;
    }
    return 1;
}

static void print(efi_system_table *system, const efi_char16 *message)
{
    if (system && system->console_out && system->console_out->output_string)
        (void)system->console_out->output_string(
            system->console_out, (efi_char16 *)message);
}

static efi_status fail(efi_system_table *system, efi_status status)
{
    print(system, failed);
    return status ? status : EFI_ERROR_BIT;
}

efi_status nux_boot_manager_main(efi_handle image, efi_system_table *system)
{
    efi_loaded_image *loaded = (efi_loaded_image *)0;
    efi_simple_file_system *filesystem = (efi_simple_file_system *)0;
    efi_file_protocol *root = (efi_file_protocol *)0;
    efi_file_protocol *kernel = (efi_file_protocol *)0;
    efi_handle kernel_image = (efi_handle)0;
    void *image_buffer = (void *)0;
    efi_u64 image_size = 0;
    efi_uintn read_size;
    efi_status status;

    if (!system || system->header.signature != EFI_SYSTEM_TABLE_SIGNATURE ||
        !system->boot_services)
        return EFI_ERROR_BIT;

    print(system, banner);
    status = system->boot_services->handle_protocol(
        image, &loaded_image_guid, (void **)&loaded);
    if (EFI_ERROR(status) || !loaded)
        return fail(system, status);

    status = system->boot_services->handle_protocol(
        loaded->device_handle, &simple_file_system_guid, (void **)&filesystem);
    if (EFI_ERROR(status) || !filesystem)
        return fail(system, status);

    status = filesystem->open_volume(filesystem, &root);
    if (EFI_ERROR(status) || !root)
        return fail(system, status);

    status = root->open(root, &kernel, (efi_char16 *)kernel_path,
                        EFI_OPEN_READ, 0);
    if (EFI_ERROR(status) || !kernel) {
        (void)root->close(root);
        return fail(system, status);
    }

    status = kernel->set_position(kernel, ~0ULL);
    if (!EFI_ERROR(status))
        status = kernel->get_position(kernel, &image_size);
    if (EFI_ERROR(status) || image_size == 0 ||
        image_size > EFI_MAX_IMAGE_SIZE) {
        (void)kernel->close(kernel);
        (void)root->close(root);
        return fail(system, status);
    }

    status = system->boot_services->allocate_pool(
        EFI_ALLOCATE_LOADER_DATA, (efi_uintn)image_size, &image_buffer);
    if (EFI_ERROR(status) || !image_buffer) {
        (void)kernel->close(kernel);
        (void)root->close(root);
        return fail(system, status);
    }

    status = kernel->set_position(kernel, 0);
    read_size = (efi_uintn)image_size;
    if (!EFI_ERROR(status))
        status = kernel->read(kernel, &read_size, image_buffer);
    (void)kernel->close(kernel);
    (void)root->close(root);
    if (EFI_ERROR(status) || read_size != (efi_uintn)image_size) {
        (void)system->boot_services->free_pool(image_buffer);
        return fail(system, status);
    }

    if (!valid_kernel_image((const efi_u8 *)image_buffer, image_size)) {
        (void)system->boot_services->free_pool(image_buffer);
        print(system, invalid_image);
        return fail(system, EFI_ERROR_BIT | 1ULL);
    }

    print(system, loading);
    status = system->boot_services->load_image(
        0, image, (void *)0, image_buffer, read_size, &kernel_image);
    (void)system->boot_services->free_pool(image_buffer);
    if (EFI_ERROR(status) || !kernel_image)
        return fail(system, status);

    status = system->boot_services->start_image(kernel_image, (efi_uintn *)0,
                                                (efi_char16 **)0);
    return fail(system, status);
}
