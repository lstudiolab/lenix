.section .text
.global nux_arch_user_enter
.type nux_arch_user_enter,@function
nux_arch_user_enter:
    movq %rdi, %r12
    movq %rsi, %r13
    movq %rdx, %r14
    movq %rcx, %r15
    subq $8, %rsp
    movq %r15, %rdi
    call nux_arch_set_kernel_stack
    addq $8, %rsp
    movq %r15, %rsp
    andq $-16, %rsp
    movq %r14, %rdi
    movw $0x1b, %cx
    movw %cx, %ds
    movw %cx, %es
    pushq $0x1b
    pushq %r13
    pushq $0x2
    pushq $0x23
    pushq %r12
    movq %cr0, %rax
    orq $0x8, %rax
    movq %rax, %cr0
    xorl %eax, %eax
    xorl %edx, %edx
    movl $0xc0000100, %ecx
    wrmsr
    movl $0xc0000101, %ecx
    wrmsr
    movl $0xc0000102, %ecx
    wrmsr
    xorq %rax, %rax
    xorq %rcx, %rcx
    xorq %rdx, %rdx
    xorq %rbx, %rbx
    xorq %rbp, %rbp
    xorq %rsi, %rsi
    xorq %r8, %r8
    xorq %r9, %r9
    xorq %r10, %r10
    xorq %r11, %r11
    xorq %r12, %r12
    xorq %r13, %r13
    xorq %r14, %r14
    xorq %r15, %r15
    iretq
.size nux_arch_user_enter, .-nux_arch_user_enter

.global nux_arch_load_gdt
.type nux_arch_load_gdt,@function
nux_arch_load_gdt:
    lgdt (%rdi)
    pushq $0x08
    leaq 1f(%rip), %rax
    pushq %rax
    lretq
1:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    ltr %si
    ret
.size nux_arch_load_gdt, .-nux_arch_load_gdt

.section .note.GNU-stack,"",@progbits
