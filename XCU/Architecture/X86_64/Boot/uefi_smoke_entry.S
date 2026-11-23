.text
.globl efi_smoke_entry
efi_smoke_entry:
    subq $40, %rsp
    call nux_boot_smoke_main
    addq $40, %rsp
    ret
