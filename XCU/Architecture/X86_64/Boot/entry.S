.section .text.boot,"ax"
.global _start
.type _start,@function
.extern nux_arch_early_init
.extern nux_kernel_main
.extern limine_memmap_request
.extern limine_hhdm_request
.extern limine_rsdp_request

_start:
    movb $0x45, %al
    outb %al, $0xe9
    cli
    cld
    andq $-16, %rsp
    call nux_arch_early_init
    movb $0x41, %al
    outb %al, $0xe9

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
    movq limine_rsdp_request+40(%rip), %rax
    testq %rax, %rax
    jz 5f
    movq 8(%rax), %rdx
    movq %rdx, 24(%rsp)
    jmp 6f
5:
    movq $0, 24(%rsp)
6:
    movq $0, 32(%rsp)
    movq %rsp, %rdi
    call nux_kernel_main
7:
    cli
    hlt
    jmp 7b
.size _start, .-_start


/* First-party UEFI handoff: RDI points to the versioned Nux boot record. */
.global _start_uefi
.type _start_uefi,@function
_start_uefi:
    cli
    cld
    movq %rdi, %r12
    andq $-16, %rsp
    call nux_arch_early_init
    movq %r12, %rdi
    call nux_kernel_main
8:
    cli
    hlt
    jmp 8b
.size _start_uefi, .-_start_uefi

.section .note.GNU-stack,"",@progbits
