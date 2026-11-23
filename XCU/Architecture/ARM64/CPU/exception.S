.section .text
.balign 2048
.global nux_exception_vectors
.type nux_exception_vectors,%function
nux_exception_vectors:
    b .Lnux_vector_0
    .space 124
    b .Lnux_vector_1
    .space 124
    b .Lnux_vector_2
    .space 124
    b .Lnux_vector_3
    .space 124
    b .Lnux_vector_4
    .space 124
    b .Lnux_vector_5
    .space 124
    b .Lnux_vector_6
    .space 124
    b .Lnux_vector_7
    .space 124
    b .Lnux_vector_8
    .space 124
    b .Lnux_vector_9
    .space 124
    b .Lnux_vector_10
    .space 124
    b .Lnux_vector_11
    .space 124
    b .Lnux_vector_12
    .space 124
    b .Lnux_vector_13
    .space 124
    b .Lnux_vector_14
    .space 124
    b .Lnux_vector_15
    .space 124
.Lnux_vector_0:
    mov x0, #0
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_1:
    mov x0, #1
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_2:
    mov x0, #2
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_3:
    mov x0, #3
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_4:
    mov x0, #4
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_5:
    mov x0, #5
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_6:
    mov x0, #6
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_7:
    mov x0, #7
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_8:
    mov x0, #8
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_9:
    mov x0, #9
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_10:
    mov x0, #10
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_11:
    mov x0, #11
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_12:
    mov x0, #12
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_13:
    mov x0, #13
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_14:
    mov x0, #14
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.Lnux_vector_15:
    mov x0, #15
    mrs x1, esr_el1
    mrs x2, far_el1
    b nux_arch_exception_dispatch
.size nux_exception_vectors, .-nux_exception_vectors
.section .note.GNU-stack,"",%progbits
