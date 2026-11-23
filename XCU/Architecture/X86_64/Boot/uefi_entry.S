.text
.globl efi_entry
efi_entry:
    subq $40, %rsp
    call nux_boot_manager_main
    addq $40, %rsp
    ret
