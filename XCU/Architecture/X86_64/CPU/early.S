.section .text
.global nux_arch_early_init
.type nux_arch_early_init,@function
nux_arch_early_init:
    cli
    cld
    ret
.size nux_arch_early_init, .-nux_arch_early_init
.section .note.GNU-stack,"",@progbits
