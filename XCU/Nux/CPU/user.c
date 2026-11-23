#include <nux/user.h>

#if defined(__x86_64__)
typedef struct __attribute__((packed)) {
    nux_u32 reserved0;
    nux_u64 rsp0, rsp1, rsp2;
    nux_u64 reserved1;
    nux_u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    nux_u64 reserved2;
    nux_u16 reserved3;
    nux_u16 iomap_base;
} nux_x86_tss;

typedef struct __attribute__((packed)) {
    nux_u16 limit;
    nux_u64 base;
} nux_x86_gdtr;

static nux_u64 gdt[7] __attribute__((aligned(16)));
static nux_x86_tss tss __attribute__((aligned(16)));
extern void nux_arch_load_gdt(const nux_x86_gdtr *descriptor,
                              nux_u16 tss_selector);

void nux_arch_user_init(void)
{
    nux_x86_gdtr descriptor;
    nux_u64 base = (nux_u64)(nux_uptr)&tss;
    nux_u64 limit = sizeof(tss) - 1;
    gdt[0] = 0;
    gdt[1] = 0x00af9a000000ffffULL;
    gdt[2] = 0x00cf92000000ffffULL;
    gdt[3] = 0x00cff2000000ffffULL;
    gdt[4] = 0x00affa000000ffffULL;
    gdt[5] = (limit & 0xffffULL) |
        ((base & 0xffffffULL) << 16) |
        (0x89ULL << 40) |
        (((limit >> 16) & 0xfULL) << 48) |
        (((base >> 24) & 0xffULL) << 56);
    gdt[6] = base >> 32;
    tss.rsp0 = 0;
    tss.iomap_base = sizeof(tss);
    descriptor.limit = (nux_u16)(sizeof(gdt) - 1);
    descriptor.base = (nux_u64)(nux_uptr)gdt;
    nux_arch_load_gdt(&descriptor, 0x28);
}

void nux_arch_user_set_kernel_stack(nux_u64 stack_top)
{
    tss.rsp0 = stack_top & ~0xfULL;
}
#elif defined(__aarch64__)
void nux_arch_user_init(void) { }
void nux_arch_user_set_kernel_stack(nux_u64 stack_top) { (void)stack_top; }
#else
#error Unsupported Nux architecture
#endif
