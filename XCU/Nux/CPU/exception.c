#include <nux/types.h>
#include <nux/exception.h>

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

extern void (*nux_isr_stub_table[256])(void);
static nux_idt_gate idt[256];

static void nux_idt_set(nux_u32 vector, void (*handler)(void), nux_u16 selector)
{
    nux_u64 address = (nux_u64)(nux_uptr)handler;
    idt[vector].offset_low = (nux_u16)address;
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].attributes = 0x8e;
    idt[vector].offset_middle = (nux_u16)(address >> 16);
    idt[vector].offset_high = (nux_u32)(address >> 32);
    idt[vector].reserved = 0;
}

void nux_arch_exception_init(void)
{
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
void nux_arch_exception_init(void) { }
#else
#error Unsupported Nux architecture
#endif

volatile nux_u64 nux_last_exception_vector;
volatile nux_u64 nux_last_exception_code;
volatile nux_u64 nux_last_exception_address;

__attribute__((noreturn))
void nux_arch_exception_dispatch(nux_u64 vector, nux_u64 code, nux_u64 address)
{
    nux_last_exception_vector = vector;
    nux_last_exception_code = code;
    nux_last_exception_address = address;
#if defined(__aarch64__)
    for (;;) __asm__ volatile("msr daifset, #0xf; wfe" ::: "memory");
#elif defined(__x86_64__)
    for (;;) __asm__ volatile("cli; hlt" ::: "memory");
#endif
}
