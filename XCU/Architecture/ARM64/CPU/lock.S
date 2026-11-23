.section .text
.global nux_arch_lock
.type nux_arch_lock, %function
nux_arch_lock:
1:
    ldaxr w1, [x0]
    cbnz w1, 2f
    mov w1, #1
    stxr w2, w1, [x0]
    cbnz w2, 1b
    ret
2:
    clrex
    yield
    b 1b
.size nux_arch_lock, .-nux_arch_lock

.global nux_arch_unlock
.type nux_arch_unlock, %function
nux_arch_unlock:
    stlr wzr, [x0]
    ret
.size nux_arch_unlock, .-nux_arch_unlock
