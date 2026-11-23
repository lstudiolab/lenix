.section .text.boot, "ax"
.global _start
.type _start, %function
.extern nux_arch_early_init
.extern nux_kernel_main
.extern limine_memmap_request
.extern limine_hhdm_request
.extern limine_rsdp_request

_start:
    msr daifset, #0xf
    bl nux_arch_early_init

    /* Preserve the bootloader stack and create the C boot-info record. */
    sub sp, sp, #48
    adrp x9, limine_memmap_request
    add x9, x9, :lo12:limine_memmap_request
    ldr x10, [x9, #40]
    cbz x10, 1f
    ldr x11, [x10, #8]
    ldr x12, [x10, #16]
    str x12, [sp, #0]
    str x11, [sp, #8]
    b 2f
1:
    str xzr, [sp, #0]
    str xzr, [sp, #8]
2:
    adrp x9, limine_hhdm_request
    add x9, x9, :lo12:limine_hhdm_request
    ldr x10, [x9, #40]
    cbz x10, 3f
    ldr x11, [x10, #8]
    str x11, [sp, #16]
    b 4f
3:
    str xzr, [sp, #16]
4:
    adrp x9, limine_rsdp_request
    add x9, x9, :lo12:limine_rsdp_request
    ldr x10, [x9, #40]
    cbz x10, 5f
    ldr x11, [x10, #8]
    str x11, [sp, #24]
    b 6f
5:
    str xzr, [sp, #24]
6:
    str xzr, [sp, #32]
    mov x0, sp
    bl nux_kernel_main
5:
    wfe
    b 5b
.size _start, .-_start
