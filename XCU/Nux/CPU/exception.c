#include <nux/types.h>
#include <nux/boot.h>
#include <nux/exception.h>
#include <nux/syscall.h>
#include <nux/thread.h>
#include <nux/user.h>

#if defined(__x86_64__)
typedef struct __attribute__((packed)) {
    nux_u16 offset_low;
    nux_u16 selector;
    nux_u8 ist;
    nux_u8 attributes;
    nux_u16 offset_middle;
    nux_u32 offset_high;
    nux_u32 reserved;
} nux_idt_gate;

typedef struct __attribute__((packed)) {
    nux_u16 limit;
    nux_u64 base;
} nux_idtr;

typedef struct {
    nux_u64 r15, r14, r13, r12, r11, r10, r9, r8;
    nux_u64 rdi, rsi, rbp, rbx, rdx, rcx, rax;
    nux_u64 vector, error, rip, cs, rflags, rsp, ss;
} nux_x86_exception_frame;

extern void (*nux_isr_stub_table[256])(void);
static nux_idt_gate idt[256];

static void nux_idt_set(nux_u32 vector, void (*handler)(void),
                       nux_u16 selector)
{
    nux_u64 address = (nux_u64)(nux_uptr)handler;
    idt[vector].offset_low = (nux_u16)address;
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].attributes = vector == 128 ? 0xee : 0x8e;
    idt[vector].offset_middle = (nux_u16)(address >> 16);
    idt[vector].offset_high = (nux_u32)(address >> 32);
    idt[vector].reserved = 0;
}

void nux_arch_exception_init(void)
{
    nux_arch_user_init();
    nux_u16 selector;
    nux_idtr descriptor;
    __asm__ volatile("mov %%cs, %0" : "=r"(selector));
    for (nux_u32 i = 0; i < 256; ++i)
        nux_idt_set(i, nux_isr_stub_table[i], selector);
    descriptor.limit = (nux_u16)(sizeof(idt) - 1);
    descriptor.base = (nux_u64)(nux_uptr)idt;
    __asm__ volatile("lidt %0" : : "m"(descriptor));
}
#elif defined(__aarch64__)
typedef struct {
    nux_u64 x[31];
    nux_u64 elr, spsr, esr, far, vector;
} nux_arm64_exception_frame;

void nux_arch_exception_init(void)
{
    nux_arch_user_init();
}
#else
#error Unsupported Nux architecture
#endif

volatile nux_u64 nux_last_exception_vector;
volatile nux_u64 nux_last_exception_code;
volatile nux_u64 nux_last_exception_address;

static void nux_arch_dispatch_user_syscall(nux_u64 number,
                                           const nux_u64 arguments[6],
                                           nux_u64 *return_value)
{
    nux_u64 result = 0;
    nux_status status = nux_syscall_dispatch(number,
        (const nux_syscall_word *)arguments, &result);
    *return_value = status == NUX_OK ? result : (nux_u64)(nux_i64)status;
}

void nux_arch_exception_dispatch(nux_u64 vector, nux_u64 code,
                                 nux_u64 address, void *frame)
{
#if defined(__x86_64__)
    if (vector == 128 && frame) {
        nux_boot_mark(NUX_BOOT_PHASE_USER_SYSCALL);
        nux_x86_exception_frame *state = (nux_x86_exception_frame *)frame;
        nux_u64 arguments[6] = {
            state->rdi, state->rsi, state->rdx,
            state->r10, state->r8, state->r9
        };
        nux_arch_dispatch_user_syscall(state->rax, arguments, &state->rax);
        return;
    }
#elif defined(__aarch64__)
    if (vector == 8 && frame && ((code >> 26) & 0x3fu) == 0x15u) {
        nux_boot_mark(NUX_BOOT_PHASE_USER_SYSCALL);
        nux_arm64_exception_frame *state =
            (nux_arm64_exception_frame *)frame;
        nux_u64 arguments[6] = {
            state->x[0], state->x[1], state->x[2],
            state->x[3], state->x[4], state->x[5]
        };
        nux_arch_dispatch_user_syscall(state->x[8], arguments,
                                       &state->x[0]);
        return;
    }
#endif

    nux_last_exception_vector = vector;
    nux_last_exception_code = code;
    nux_last_exception_address = address;

#if defined(__x86_64__)
    if (frame && (((nux_x86_exception_frame *)frame)->cs & 3u) == 3u) {
        nux_boot_mark(NUX_BOOT_PHASE_USER_FAULT);
        (void)nux_thread_exit_current();
        for (;;) __asm__ volatile("cli; hlt" ::: "memory");
    }
    if (vector >= 32) return;
    for (;;) __asm__ volatile("cli; hlt" ::: "memory");
#elif defined(__aarch64__)
    if (frame && ((((nux_arm64_exception_frame *)frame)->spsr & 0xfu) == 0u)) {
        nux_boot_mark(NUX_BOOT_PHASE_USER_FAULT);
        (void)nux_thread_exit_current();
        for (;;) __asm__ volatile("msr daifset, #0xf; wfe" ::: "memory");
    }
    for (;;) __asm__ volatile("msr daifset, #0xf; wfe" ::: "memory");
#endif
}
