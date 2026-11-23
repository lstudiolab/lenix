.text
.globl efi_kernel_entry
efi_kernel_entry:
    subq $40, %rsp
    call nux_kernel_loader_main
    addq $40, %rsp
    ret
