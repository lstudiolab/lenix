.text
.globl efi_kernel_entry
efi_kernel_entry:
    movq %rcx, %rdi
    movq %rdx, %rsi
    subq $8, %rsp
    call nux_uefi_kernel_main
    addq $8, %rsp
    ret

.globl nux_arch_enter_kernel
nux_arch_enter_kernel:
    movq %rdi, %r12
    movq %rsi, %r13
    movq %rcx, %r14
    movq %rdx, %rsp
    andq $-16, %rsp
    call *%r14
    movq %r13, %rdi
    call *%r12
1:
    cli
    hlt
    jmp 1b
