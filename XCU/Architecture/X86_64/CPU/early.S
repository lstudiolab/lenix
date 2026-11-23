.section .text
.global nux_arch_early_init
.type nux_arch_early_init,@function
.extern nux_arch_exception_init
nux_arch_early_init:
    cli
    cld
    subq $8, %rsp
    call nux_arch_exception_init
    addq $8, %rsp
    ret
.size nux_arch_early_init, .-nux_arch_early_init
.section .note.GNU-stack,"",@progbits
