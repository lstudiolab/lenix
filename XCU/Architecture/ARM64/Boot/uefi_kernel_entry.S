.text
.globl efi_kernel_entry
efi_kernel_entry:
    b nux_uefi_kernel_main

.globl nux_arch_enter_kernel
nux_arch_enter_kernel:
    mov x19, x0
    mov x20, x1
    mov sp, x2
    blr x3
    mov x0, x20
    br x19
