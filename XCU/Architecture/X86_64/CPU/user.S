.section .text
.global nux_arch_user_enter
.type nux_arch_user_enter,@function
nux_arch_user_enter:
    movq %rdi, %rax
    movq %rdx, %rdi
    movw $0x1b, %cx
    movw %cx, %ds
    movw %cx, %es
    pushq $0x1b
    pushq %rsi
    pushq $0x2
    pushq $0x23
    pushq %rax
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
