.section .text
.global nux_arch_user_enter
.type nux_arch_user_enter,%function
nux_arch_user_enter:
    msr sp_el0, x1
    mov x3, x2
    msr elr_el1, x0
    msr spsr_el1, xzr
    mov x0, x3
    isb
    eret
.size nux_arch_user_enter, .-nux_arch_user_enter
.global nux_arch_set_kernel_stack
.type nux_arch_set_kernel_stack,%function
nux_arch_set_kernel_stack:
    ret
.size nux_arch_set_kernel_stack, .-nux_arch_set_kernel_stack
