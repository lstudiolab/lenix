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
