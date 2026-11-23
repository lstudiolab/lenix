#ifndef LENIX_UEFI_H
#define LENIX_UEFI_H

#if defined(__x86_64__)
#define LENIX_EFIAPI __attribute__((ms_abi))
#else
#define LENIX_EFIAPI
#endif

typedef unsigned char  efi_u8;
typedef unsigned short efi_u16;
typedef unsigned int   efi_u32;
typedef unsigned long long efi_u64;
typedef unsigned long long efi_uintn;
typedef efi_u64 efi_status;
typedef void *efi_handle;
typedef efi_u16 efi_char16;

#define EFI_SUCCESS 0ULL
#define EFI_ERROR_BIT (1ULL << 63)
#define EFI_ERROR(status) (((status) & EFI_ERROR_BIT) != 0)
#define EFI_OPEN_READ 1ULL
#define EFI_ALLOCATE_LOADER_DATA 2u
#define EFI_MAX_IMAGE_SIZE (256ULL * 1024ULL * 1024ULL)

typedef struct {
    efi_u32 data1;
    efi_u16 data2;
    efi_u16 data3;
    efi_u8 data4[8];
} efi_guid;

typedef struct {
    efi_u64 signature;
    efi_u32 revision;
    efi_u32 header_size;
    efi_u32 crc32;
    efi_u32 reserved;
} efi_table_header;

struct efi_file_protocol;
typedef struct efi_file_protocol efi_file_protocol;

typedef efi_status (LENIX_EFIAPI *efi_file_open)(
    efi_file_protocol *, efi_file_protocol **, efi_char16 *, efi_u64, efi_u64);
typedef efi_status (LENIX_EFIAPI *efi_file_close)(efi_file_protocol *);
typedef efi_status (LENIX_EFIAPI *efi_file_read)(
    efi_file_protocol *, efi_uintn *, void *);
typedef efi_status (LENIX_EFIAPI *efi_file_get_position)(
    efi_file_protocol *, efi_u64 *);
typedef efi_status (LENIX_EFIAPI *efi_file_set_position)(
    efi_file_protocol *, efi_u64 *);

struct efi_file_protocol {
    efi_u64 revision;
    efi_file_open open;
    efi_file_close close;
    void *delete_file;
    efi_file_read read;
    void *write;
    efi_file_get_position get_position;
    efi_file_set_position set_position;
    void *get_info;
    void *set_info;
    void *flush;
};

typedef struct {
    efi_u64 revision;
    efi_status (LENIX_EFIAPI *open_volume)(void *, efi_file_protocol **);
} efi_simple_file_system;

typedef struct {
    efi_u32 revision;
    efi_handle parent_handle;
    void *system_table;
    efi_handle device_handle;
    void *file_path;
    void *reserved;
    efi_u32 load_options_size;
    void *load_options;
    void *image_base;
    efi_u64 image_size;
    efi_u32 image_code_type;
    efi_u32 image_data_type;
    void *unload;
} efi_loaded_image;

typedef struct {
    void *reset;
    efi_status (LENIX_EFIAPI *output_string)(void *, efi_char16 *);
} efi_text_output;

typedef efi_status (LENIX_EFIAPI *efi_handle_protocol)(
    efi_handle, const efi_guid *, void **);
typedef efi_status (LENIX_EFIAPI *efi_allocate_pool)(
    efi_u32, efi_uintn, void **);
typedef efi_status (LENIX_EFIAPI *efi_free_pool)(void *);
typedef efi_status (LENIX_EFIAPI *efi_load_image)(
    efi_u8, efi_handle, void *, void *, efi_uintn, efi_handle *);
typedef efi_status (LENIX_EFIAPI *efi_start_image)(
    efi_handle, efi_uintn *, efi_char16 **);

typedef struct {
    efi_table_header header;
    void *raise_tpl;
    void *restore_tpl;
    void *allocate_pages;
    void *free_pages;
    void *get_memory_map;
    efi_allocate_pool allocate_pool;
    efi_free_pool free_pool;
    void *create_event;
    void *set_timer;
    void *wait_for_event;
    void *signal_event;
    void *close_event;
    void *check_event;
    void *install_protocol_interface;
    void *reinstall_protocol_interface;
    void *uninstall_protocol_interface;
    efi_handle_protocol handle_protocol;
    void *reserved;
    void *register_protocol_notify;
    void *locate_handle;
    void *locate_device_path;
    void *install_configuration_table;
    efi_load_image load_image;
    efi_start_image start_image;
} efi_boot_services_prefix;

typedef struct {
    efi_table_header header;
    efi_char16 *firmware_vendor;
    efi_u32 firmware_revision;
    efi_u32 padding;
    efi_handle console_in_handle;
    void *console_in;
    efi_handle console_out_handle;
    efi_text_output *console_out;
    efi_handle standard_error_handle;
    efi_text_output *standard_error;
    void *runtime_services;
    efi_boot_services_prefix *boot_services;
    efi_uintn number_of_table_entries;
    void *configuration_table;
} efi_system_table;

#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249ULL

#endif
