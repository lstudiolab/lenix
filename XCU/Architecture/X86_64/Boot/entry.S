.section .text.boot,"ax"
.global _start
.global _start_uefi
.type _start,@function
.type _start_uefi,@function
.extern nux_arch_early_init
.extern nux_kernel_main

/* Both ELF entry names now use the first-party UEFI handoff ABI. */
_start:
_start_uefi:
    cli
    cld
    movq %rdi, %r12
    andq $-16, %rsp
    call nux_arch_early_init
    movq %r12, %rdi
    call nux_kernel_main
1:
    cli
    hlt
    jmp 1b
.size _start, .-_start
.size _start_uefi, .-_start_uefi

.section .note.GNU-stack,"",@progbits
