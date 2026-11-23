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
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_1:
    mov x0, #1
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_2:
    mov x0, #2
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_3:
    mov x0, #3
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_4:
    mov x0, #4
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_5:
    mov x0, #5
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_6:
    mov x0, #6
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_7:
    mov x0, #7
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_8:
    msr spsel, #1
    sub sp, sp, #288
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    mrs x9, elr_el1
    str x9, [sp, #248]
    mrs x9, spsr_el1
    str x9, [sp, #256]
    mrs x9, esr_el1
    str x9, [sp, #264]
    mrs x9, far_el1
    str x9, [sp, #272]
    mov x9, #8
    str x9, [sp, #280]
    mov x0, #8
    ldr x1, [sp, #264]
    ldr x2, [sp, #272]
    mov x3, sp
    bl nux_arch_exception_dispatch
    ldr x9, [sp, #248]
    msr elr_el1, x9
    ldr x9, [sp, #256]
    msr spsr_el1, x9
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]
    add sp, sp, #288
    eret
.Lnux_vector_9:
    mov x0, #9
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_10:
    mov x0, #10
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_11:
    mov x0, #11
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_12:
    mov x0, #12
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_13:
    mov x0, #13
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_14:
    mov x0, #14
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch
.Lnux_vector_15:
    mov x0, #15
    mrs x1, esr_el1
    mrs x2, far_el1
    mov x3, xzr
    b nux_arch_exception_dispatch


.size nux_exception_vectors, .-nux_exception_vectors
.section .note.GNU-stack,"",%progbits
