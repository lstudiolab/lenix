.section .text
.global nux_arch_enter_user
.type nux_arch_enter_user,%function
nux_arch_enter_user:
    mov sp, x3
    msr sp_el0, x1
    mov x3, x2
    msr elr_el1, x0
    mov x4, #0x3c0
    msr spsr_el1, x4
    mov x0, x3
    isb
    eret
.size nux_arch_enter_user, .-nux_arch_enter_user

.section .note.GNU-stack,"",%progbits
