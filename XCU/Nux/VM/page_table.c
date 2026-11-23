#include <nux/types.h>
#include <nux/status.h>
#include <nux/memory.h>

extern void *memset(void *dst,int value,nux_size size);

static nux_u64 *table_ptr(nux_u64 pa) {
    return (nux_u64 *)nux_phys_to_virt(pa);
}
static nux_u64 alloc_table(void) {
    nux_u64 pa=nux_phys_alloc_page();
    if(pa)memset(nux_phys_to_virt(pa),0,NUX_PAGE_SIZE);
    return pa;
}

#if defined(__x86_64__)
#define X86_P 1ULL
#define X86_RW 2ULL
#define X86_US 4ULL
#define X86_NX (1ULL<<63)
#define X86_ADDR 0x000ffffffffff000ULL
static nux_u64 kernel_root;
static nux_u64 current_root(void){nux_u64 r;__asm__ volatile("mov %%cr3,%0":"=r"(r));return r&X86_ADDR;}
static void invalidate(nux_u64 va){nux_u64 r=current_root();(void)r;__asm__ volatile("invlpg (%0)"::"r"(va):"memory");}
nux_status nux_arch_vm_root_create(nux_u64 *out) {
    if(!out)return NUX_E_INVALID;
    nux_u64 root=alloc_table();if(!root)return NUX_E_NO_MEMORY;
    if(!kernel_root){
        kernel_root=current_root();
        nux_u32 lo,hi;__asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xc0000080));
        lo|=1u<<11;__asm__ volatile("wrmsr"::"a"(lo),"d"(hi),"c"(0xc0000080));
    }
    nux_u64 *dst=table_ptr(root),*src=table_ptr(kernel_root);
    if(!dst||!src){(void)nux_phys_free_page(root);return NUX_E_NO_MEMORY;}
    for(nux_u32 i=256;i<512;i++)dst[i]=src[i];
    *out=root;return NUX_OK;
}
nux_status nux_arch_vm_map(nux_u64 root,nux_u64 va,nux_u64 pa,nux_u32 prot) {
    if((va&4095)||(pa&4095)||va>=0x0000800000000000ULL)return NUX_E_INVALID;
    nux_u64 *t=table_ptr(root);if(!t)return NUX_E_INVALID;
    const nux_u32 shifts[3]={39,30,21};
    for(nux_u32 l=0;l<3;l++){
        nux_u32 i=(nux_u32)((va>>shifts[l])&511);
        if(!(t[i]&X86_P)){nux_u64 child=alloc_table();if(!child)return NUX_E_NO_MEMORY;t[i]=child|X86_P|X86_RW|X86_US;}
        else t[i]|=X86_RW|X86_US;
        t=table_ptr(t[i]&X86_ADDR);if(!t)return NUX_E_NO_MEMORY;
    }
    nux_u32 i=(nux_u32)((va>>12)&511);if(t[i]&X86_P)return NUX_E_BUSY;
    t[i]=(pa&X86_ADDR)|X86_P|X86_US|((prot&2)?X86_RW:0)|((prot&4)?0:X86_NX);
    invalidate(va);return NUX_OK;
}
nux_status nux_arch_vm_unmap(nux_u64 root,nux_u64 va) {
    nux_u64 *t=table_ptr(root);if(!t)return NUX_E_INVALID;
    const nux_u32 shifts[3]={39,30,21};
    for(nux_u32 l=0;l<3;l++){nux_u64 e=t[(va>>shifts[l])&511];if(!(e&X86_P))return NUX_E_NOT_FOUND;t=table_ptr(e&X86_ADDR);if(!t)return NUX_E_NO_MEMORY;}
    nux_u32 i=(nux_u32)((va>>12)&511);if(!(t[i]&X86_P))return NUX_E_NOT_FOUND;t[i]=0;invalidate(va);return NUX_OK;
}
nux_status nux_arch_vm_protect(nux_u64 root,nux_u64 va,nux_u32 prot) {
    nux_u64 *t=table_ptr(root);if(!t)return NUX_E_INVALID;
    const nux_u32 shifts[3]={39,30,21};
    for(nux_u32 l=0;l<3;l++){nux_u64 e=t[(va>>shifts[l])&511];if(!(e&X86_P))return NUX_E_NOT_FOUND;t=table_ptr(e&X86_ADDR);if(!t)return NUX_E_NO_MEMORY;}
    nux_u32 i=(nux_u32)((va>>12)&511);if(!(t[i]&X86_P))return NUX_E_NOT_FOUND;
    t[i]=(t[i]&~(X86_RW|X86_NX))|((prot&2)?X86_RW:0)|((prot&4)?0:X86_NX);invalidate(va);return NUX_OK;
}
nux_u64 nux_arch_vm_translate(nux_u64 root,nux_u64 va) {
    nux_u64 *t=table_ptr(root);if(!t)return 0;const nux_u32 shifts[4]={39,30,21,12};
    for(nux_u32 l=0;l<4;l++){nux_u64 e=t[(va>>shifts[l])&511];if(!(e&X86_P))return 0;if(l==3)return (e&X86_ADDR)|(va&4095);t=table_ptr(e&X86_ADDR);if(!t)return 0;}return 0;
}
static void free_tree(nux_u64 pa,nux_u32 level) {
    nux_u64 *t=table_ptr(pa);if(!t)return;
    if(level<3)for(nux_u32 i=0;i<512;i++)if(t[i]&X86_P)free_tree(t[i]&X86_ADDR,level+1);
    (void)nux_phys_free_page(pa);
}
void nux_arch_vm_root_destroy(nux_u64 root) {
    nux_u64 *t=table_ptr(root);if(!t)return;
    for(nux_u32 i=0;i<256;i++)if(t[i]&X86_P)free_tree(t[i]&X86_ADDR,1);
    (void)nux_phys_free_page(root);
}
nux_status nux_arch_vm_activate(nux_u64 root) {
    if(root&&(root&4095))return NUX_E_INVALID;
    if(!root)root=kernel_root;
    if(!root)return NUX_E_BAD_STATE;
    __asm__ volatile("mov %0,%%cr3"::"r"(root):"memory");return NUX_OK;
}
#elif defined(__aarch64__)
#define ARM_ADDR 0x0000fffffffff000ULL
#define ARM_VALID 3ULL
#define ARM_AF (1ULL<<10)
#define ARM_SH_INNER (3ULL<<8)
#define ARM_AP_USER_RW (1ULL<<6)
#define ARM_AP_USER_RO (3ULL<<6)
#define ARM_PXN (1ULL<<53)
#define ARM_UXN (1ULL<<54)
static nux_u64 kernel_root;
static void invalidate(nux_u64 va) {
    nux_u64 operand=va>>12;
    __asm__ volatile("dsb ishst; tlbi vae1is, %0; dsb ish; isb"::"r"(operand):"memory");
}
nux_status nux_arch_vm_root_create(nux_u64 *out) {
    if(!out)return NUX_E_INVALID;
    if(!kernel_root){
        nux_u64 tcr,mair;
        __asm__ volatile("mrs %0, tcr_el1":"=r"(tcr));
        tcr=(tcr&~0x7fbfULL)|16ULL|(1ULL<<8)|(1ULL<<10)|(3ULL<<12);
        __asm__ volatile("msr tcr_el1, %0; isb"::"r"(tcr):"memory");
        __asm__ volatile("mrs %0, mair_el1":"=r"(mair));mair=(mair&~0xffULL)|0xffULL;
        __asm__ volatile("msr mair_el1, %0; isb"::"r"(mair):"memory");

        /*
         * The kernel executes through TTBR1_EL1. TTBR0_EL1 is the isolated
         * user translation regime, so the kernel/default root must be empty;
         * preserving the firmware's TTBR0 would leak its identity mappings.
         */
        kernel_root=alloc_table();
        if(!kernel_root)return NUX_E_NO_MEMORY;
    }
    nux_u64 root=alloc_table();if(!root)return NUX_E_NO_MEMORY;*out=root;return NUX_OK;
}
nux_status nux_arch_vm_map(nux_u64 root,nux_u64 va,nux_u64 pa,nux_u32 prot) {
    if((va&4095)||(pa&4095)||va>=0x0001000000000000ULL)return NUX_E_INVALID;
    nux_u64 *t=table_ptr(root);if(!t)return NUX_E_INVALID;
    const nux_u32 shifts[3]={39,30,21};
    for(nux_u32 l=0;l<3;l++){nux_u32 i=(nux_u32)((va>>shifts[l])&511);if(!(t[i]&1)){nux_u64 child=alloc_table();if(!child)return NUX_E_NO_MEMORY;t[i]=child|ARM_VALID;}t=table_ptr(t[i]&ARM_ADDR);if(!t)return NUX_E_NO_MEMORY;}
    nux_u32 i=(nux_u32)((va>>12)&511);if(t[i]&1)return NUX_E_BUSY;
    nux_u64 desc=(pa&ARM_ADDR)|ARM_VALID|ARM_AF|ARM_SH_INNER|((prot&2)?ARM_AP_USER_RW:ARM_AP_USER_RO)|ARM_PXN;
    if(!(prot&4))desc|=ARM_UXN;
    t[i]=desc;invalidate(va);return NUX_OK;
}
nux_status nux_arch_vm_unmap(nux_u64 root,nux_u64 va) {
    nux_u64 *t=table_ptr(root);if(!t)return NUX_E_INVALID;const nux_u32 shifts[3]={39,30,21};
    for(nux_u32 l=0;l<3;l++){nux_u64 e=t[(va>>shifts[l])&511];if(!(e&1))return NUX_E_NOT_FOUND;t=table_ptr(e&ARM_ADDR);if(!t)return NUX_E_NO_MEMORY;}
    nux_u32 i=(nux_u32)((va>>12)&511);if(!(t[i]&1))return NUX_E_NOT_FOUND;t[i]=0;invalidate(va);return NUX_OK;
}
nux_status nux_arch_vm_protect(nux_u64 root,nux_u64 va,nux_u32 prot) {
    nux_u64 *t=table_ptr(root);if(!t)return NUX_E_INVALID;const nux_u32 shifts[3]={39,30,21};
    for(nux_u32 l=0;l<3;l++){nux_u64 e=t[(va>>shifts[l])&511];if(!(e&1))return NUX_E_NOT_FOUND;t=table_ptr(e&ARM_ADDR);if(!t)return NUX_E_NO_MEMORY;}
    nux_u32 i=(nux_u32)((va>>12)&511);if(!(t[i]&1))return NUX_E_NOT_FOUND;
    t[i]=(t[i]&~((3ULL<<6)|ARM_UXN))|((prot&2)?ARM_AP_USER_RW:ARM_AP_USER_RO)|((prot&4)?0:ARM_UXN);invalidate(va);return NUX_OK;
}
nux_u64 nux_arch_vm_translate(nux_u64 root,nux_u64 va) {
    nux_u64 *t=table_ptr(root);if(!t)return 0;const nux_u32 shifts[4]={39,30,21,12};
    for(nux_u32 l=0;l<4;l++){nux_u64 e=t[(va>>shifts[l])&511];if(!(e&1))return 0;if(l==3)return (e&ARM_ADDR)|(va&4095);t=table_ptr(e&ARM_ADDR);if(!t)return 0;}return 0;
}
static void free_tree(nux_u64 pa,nux_u32 level) {
    nux_u64 *t=table_ptr(pa);if(!t)return;
    if(level<3)for(nux_u32 i=0;i<512;i++)if(t[i]&1)free_tree(t[i]&ARM_ADDR,level+1);
    (void)nux_phys_free_page(pa);
}
void nux_arch_vm_root_destroy(nux_u64 root) { if(root)free_tree(root,0); }
nux_status nux_arch_vm_activate(nux_u64 root) {
    if(root&&(root&4095))return NUX_E_INVALID;
    if(!root)root=kernel_root;
    __asm__ volatile("dsb ishst; msr ttbr0_el1, %0; tlbi vmalle1is; dsb ish; isb"::"r"(root):"memory");return NUX_OK;
}
#else
#error Unsupported Nux architecture
#endif
