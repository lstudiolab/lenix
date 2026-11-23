.text
.globl efi_kernel_entry
efi_kernel_entry:
    b nux_kernel_loader_main
