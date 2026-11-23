.section .text
.global nux_arch_lock
.type nux_arch_lock,@function
nux_arch_lock:
    movl $1, %eax
1:
    xchgl %eax, (%rdi)
    testl %eax, %eax
    jnz 1b
    ret
.size nux_arch_lock, .-nux_arch_lock

.global nux_arch_unlock
.type nux_arch_unlock,@function
nux_arch_unlock:
    movl $0, (%rdi)
    ret
.size nux_arch_unlock, .-nux_arch_unlock

.section .note.GNU-stack,"",@progbits
