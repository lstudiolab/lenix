.section .text
.global nux_arch_user_enter
.type nux_arch_user_enter,%function
nux_arch_user_enter:
    mov sp, x3
    msr sp_el0, x1
    mov x3, x2
    msr elr_el1, x0
    mov x4, #0x3c0
    msr spsr_el1, x4
    msr cpacr_el1, xzr
    msr tpidr_el0, xzr
    mov x0, x3
    mov x1, xzr
    mov x2, xzr
    mov x3, xzr
    mov x4, xzr
    mov x5, xzr
    mov x6, xzr
    mov x7, xzr
    mov x8, xzr
    mov x9, xzr
    mov x10, xzr
    mov x11, xzr
    mov x12, xzr
    mov x13, xzr
    mov x14, xzr
    mov x15, xzr
    mov x16, xzr
    mov x17, xzr
    mov x18, xzr
    mov x19, xzr
    mov x20, xzr
    mov x21, xzr
    mov x22, xzr
    mov x23, xzr
    mov x24, xzr
    mov x25, xzr
    mov x26, xzr
    mov x27, xzr
    mov x28, xzr
    mov x29, xzr
    mov x30, xzr
    isb
    eret
.size nux_arch_user_enter, .-nux_arch_user_enter

.section .note.GNU-stack,"",%progbits
