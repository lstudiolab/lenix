#include <nux/types.h>
#include <nux/exception.h>
#include <nux/types.h>

#if defined(__x86_64__)
typedef struct __attribute__((packed)) {
    nux_u32 reserved0;
    nux_u64 rsp0;
    nux_u64 rsp1;
    nux_u64 rsp2;
    nux_u64 reserved1;
    nux_u64 ist1;
    nux_u64 ist2;
    nux_u64 ist3;
    nux_u64 ist4;
    nux_u64 ist5;
    nux_u64 ist6;
    nux_u64 ist7;
    nux_u64 reserved2;
    nux_u16 reserved3;
    nux_u16 iomap_base;
} nux_x86_tss;

typedef struct __attribute__((packed)) {
    nux_u16 limit;
    nux_u64 base;
} nux_x86_gdtr;

static nux_u64 nux_x86_gdt[7] __attribute__((aligned(16)));
static nux_x86_tss nux_x86_tss_state __attribute__((aligned(16)));

void nux_arch_privilege_init(void)
{
    nux_x86_tss_state.iomap_base = sizeof(nux_x86_tss_state);
    nux_x86_gdt[0] = 0;
    nux_x86_gdt[1] = 0x00af9a000000ffffULL;
    nux_x86_gdt[2] = 0x00cf92000000ffffULL;
    nux_x86_gdt[3] = 0x00cff2000000ffffULL;
    nux_x86_gdt[4] = 0x00affa000000ffffULL;

    nux_u64 base = (nux_u64)(nux_uptr)&nux_x86_tss_state;
    nux_u64 limit = sizeof(nux_x86_tss_state) - 1;
    nux_x86_gdt[5] = (limit & 0xffffULL) |
        ((base & 0xffffffULL) << 16) |
        (0x89ULL << 40) |
        (((limit >> 16) & 0xfULL) << 48) |
        (((base >> 24) & 0xffULL) << 56);
    nux_x86_gdt[6] = base >> 32;

    nux_x86_gdtr descriptor = {
        .limit = sizeof(nux_x86_gdt) - 1,
        .base = (nux_u64)(nux_uptr)nux_x86_gdt
    };
    __asm__ volatile(
        "lgdt %0\n"
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        "movw $0x28, %%ax\n"
        "ltr %%ax\n"
        : : "m"(descriptor) : "rax", "memory");
}

void nux_arch_set_kernel_stack(nux_u64 stack_top)
{
    nux_x86_tss_state.rsp0 = stack_top;
}
#else
void nux_arch_privilege_init(void) { }
void nux_arch_set_kernel_stack(nux_u64 stack_top) { (void)stack_top; }
#endif



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
    nux_arch_privilege_init();
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
