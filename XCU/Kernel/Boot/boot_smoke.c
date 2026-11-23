#include "uefi.h"

static const efi_char16 message[] = {
    'N','U','X',':','E','F','I','_','S','T','A','G','E','2','_','O','K','\r','\n',0
};

efi_status nux_boot_smoke_main(efi_handle image, efi_system_table *system)
{
    (void)image;
    if (!system || system->header.signature != EFI_SYSTEM_TABLE_SIGNATURE)
        return EFI_ERROR_BIT;
    if (system->console_out && system->console_out->output_string)
        (void)system->console_out->output_string(
            system->console_out, (efi_char16 *)message);
#if defined(LENIX_QEMU_BOOT_TRACE)
#if defined(__x86_64__)
    static const char trace[] = "NUX:EFI_STAGE2_OK\n";
    for (efi_uintn i = 0; trace[i]; ++i)
        __asm__ volatile("outb %0, $0xe9" : : "a"((unsigned char)trace[i]));
#elif defined(__aarch64__)
    static const char trace[] = "NUX:EFI_STAGE2_OK\n";
    register efi_u64 operation __asm__("x0") = 0x04;
    register const char *text __asm__("x1") = trace;
    __asm__ volatile("hlt #0xf000" : "+r"(operation) : "r"(text) : "memory");
#endif
#endif
    return EFI_SUCCESS;
}
