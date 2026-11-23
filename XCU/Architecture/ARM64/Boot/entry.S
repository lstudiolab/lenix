.section .text.boot, "ax"
.global _start
.global _start_uefi
.type _start, %function
.type _start_uefi, %function
.extern nux_arch_early_init
.extern nux_kernel_main

/* Both ELF entry names now use the first-party UEFI handoff ABI. */
_start:
_start_uefi:
    mov x19, x0
#ifdef NUX_QEMU_SEMIHOST
    mov x0, #0x04
    adr x1, .Lnux_start_trace
    hlt #0xf000
#endif
    msr daifset, #0xf
    bl nux_arch_early_init
#ifdef NUX_QEMU_SEMIHOST
    mov x0, #0x04
    adr x1, .Lnux_entry_trace
    hlt #0xf000
#endif
    mov x0, x19
#ifdef NUX_QEMU_SEMIHOST
    mov x0, #0x04
    adr x1, .Lnux_main_trace
    hlt #0xf000
    mov x0, x19
#endif
    bl nux_kernel_main
1:
    wfe
    b 1b
.size _start, .-_start
.size _start_uefi, .-_start_uefi

#ifdef NUX_QEMU_SEMIHOST
.p2align 2
.Lnux_start_trace:
    .asciz "NUX:KERNEL_ENTRY\n"
.Lnux_entry_trace:
    .asciz "NUX:EARLY_INIT_RETURNED\n"
.Lnux_main_trace:
    .asciz "NUX:CALL_MAIN\n"
#endif
