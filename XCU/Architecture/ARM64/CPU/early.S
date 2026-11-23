.section .text
.global nux_arch_early_init
.type nux_arch_early_init, %function
.extern nux_exception_vectors
nux_arch_early_init:
    msr daifset, #0xf
    adrp x0, nux_exception_vectors
    add x0, x0, :lo12:nux_exception_vectors
    msr vbar_el1, x0
    dsb sy
    isb
    ret
.size nux_arch_early_init, .-nux_arch_early_init
