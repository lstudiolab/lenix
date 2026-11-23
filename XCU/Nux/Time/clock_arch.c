#include <nux/types.h>
#include <nux/status.h>
static nux_u64 clock_frequency;
nux_status nux_arch_clock_init(void) {
#if defined(__aarch64__)
    __asm__ volatile("mrs %0, cntfrq_el0":"=r"(clock_frequency));
#elif defined(__x86_64__)
    nux_u32 a,b,c,d;
    __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0),"c"(0));
    nux_u32 max_leaf=a;
    if(max_leaf>=0x15) {
        __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x15),"c"(0));
        if(a&&b&&c)clock_frequency=((nux_u64)c*b)/a;
    }
    if(!clock_frequency&&max_leaf>=0x16) {
        __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x16),"c"(0));
        if(a)clock_frequency=(nux_u64)a*1000000ULL;
    }
#else
#error Unsupported Nux architecture
#endif
    return clock_frequency?NUX_OK:NUX_E_NOT_SUPPORTED;
}
nux_u64 nux_arch_clock_frequency(void){return clock_frequency;}
nux_u64 nux_arch_clock_ticks(void) {
#if defined(__aarch64__)
    nux_u64 value;__asm__ volatile("isb; mrs %0, cntvct_el0":"=r"(value));return value;
#elif defined(__x86_64__)
    nux_u32 low,high;__asm__ volatile("rdtsc":"=a"(low),"=d"(high));return ((nux_u64)high<<32)|low;
#endif
}
