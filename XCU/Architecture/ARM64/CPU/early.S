.section .text
.global nux_arch_early_init
.type nux_arch_early_init, %function
nux_arch_early_init:
    msr daifset, #0xf
    dsb sy
    isb
    ret
.size nux_arch_early_init, .-nux_arch_early_init
