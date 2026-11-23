.section .text.boot,"ax"
.global _start
.type _start,@function
.extern nux_arch_early_init
.extern nux_kernel_main
.extern limine_memmap_request
.extern limine_hhdm_request

_start:
    cli
    cld
    andq $-16, %rsp
    call nux_arch_early_init

    /* nux_boot_info is 40 bytes; keep the call site 16-byte aligned. */
    subq $48, %rsp
    movq limine_memmap_request+40(%rip), %rax
    testq %rax, %rax
    jz 1f
    movq 8(%rax), %rcx
    movq 16(%rax), %rdx
    movq %rdx, 0(%rsp)
    movq %rcx, 8(%rsp)
    jmp 2f
1:
    movq $0, 0(%rsp)
    movq $0, 8(%rsp)
2:
    movq limine_hhdm_request+40(%rip), %rax
    testq %rax, %rax
    jz 3f
    movq 8(%rax), %rdx
    movq %rdx, 16(%rsp)
    jmp 4f
3:
    movq $0, 16(%rsp)
4:
    movq $0, 24(%rsp)
    movq $0, 32(%rsp)
    movq %rsp, %rdi
    call nux_kernel_main
5:
    cli
    hlt
    jmp 5b
.size _start, .-_start

.section .note.GNU-stack,"",@progbits
