.section .text
.global nux_arch_user_enter
.type nux_arch_user_enter,%function
nux_arch_user_enter:
    msr sp_el0, x1
    mov x3, x2
    msr elr_el1, x0
    mov x4, #0x3c0
    msr spsr_el1, x4
    mov x0, x3
    isb
    eret
.size nux_arch_user_enter, .-nux_arch_user_enter

.global nux_arch_syscall_entry
.type nux_arch_syscall_entry,%function
nux_arch_syscall_entry:
    svc #0
    ret
.size nux_arch_syscall_entry, .-nux_arch_syscall_entry
