.section .text
.global nux_arch_early_init
.type nux_arch_early_init, %function
.extern nux_exception_vectors

/*
 * Limine base revision 6 may enter at EL2 with VHE enabled. Nux normalizes
 * that handoff to EL1h before installing the EL1 exception vector table.
 */
nux_arch_early_init:
    msr daifset, #0xf
    mrs x9, CurrentEL
    cmp x9, #0x8
    b.ne 2f

    mov x10, sp
    msr sp_el1, x10
    mov x10, #0x3c5
    msr spsr_el2, x10
    adr x10, 1f
    msr elr_el2, x10

    /* Permit EL1 physical/virtual counter reads and disable EL2 VHE. */
    mov x10, #3
    msr cnthctl_el2, x10
    msr cntvoff_el2, xzr
    mov x10, #0x80000000
    msr hcr_el2, x10
    isb
    eret

1:
    msr daifset, #0xf
2:
    adrp x0, nux_exception_vectors
    add x0, x0, :lo12:nux_exception_vectors
    msr vbar_el1, x0
    dsb sy
    isb
    ret
.size nux_arch_early_init, .-nux_arch_early_init
