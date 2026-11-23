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
