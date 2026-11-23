#include <nux/processor.h>
#include <nux/acpi.h>

void nux_arch_lock(volatile nux_u32 *lock){while(*lock){}*lock=1;}
void nux_arch_unlock(volatile nux_u32 *lock){*lock=0;}
nux_status nux_thread_set_processor_set(nux_u32 thread,nux_u32 set){(void)thread;(void)set;return NUX_OK;}
nux_u32 nux_thread_processor_set_busy(nux_u32 set){(void)set;return 0;}
nux_u64 nux_processor_test_hardware_id(void){return 0x800000ULL;}
nux_status nux_acpi_get_info(nux_acpi_info *out)
{
    if (!out) return NUX_E_INVALID;
    for (nux_u32 i=0;i<sizeof(*out);++i)((nux_u8*)out)[i]=0;
    out->cpu_count=1;
    out->cpus[0].uid=9;
    out->cpus[0].hardware_id=0x800000ULL;
    out->cpus[0].flags=1;
    out->cpus[0].x2apic=2;
    return NUX_OK;
}
int main(void)
{
    nux_processor_info info;
    if(nux_processor_system_init()!=NUX_OK)return 1;
    if(nux_processor_count()!=1)return 2;
    if (nux_processor_current_id() != 0) return 19;
    if(nux_processor_get_info(0,&info)!=NUX_OK)return 3;
    if(info.architecture!=NUX_PROCESSOR_ARCH_ARM64||!info.online||
       info.logical_id!=9||info.hardware_id!=0x800000ULL)return 4;
    if(nux_processor_start(0)!=NUX_OK)return 5;
    if(nux_processor_stop(0)!=NUX_E_ACCESS)return 6;
    return 0;
}
