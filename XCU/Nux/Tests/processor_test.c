#include <nux/processor.h>
#include <nux/acpi.h>
nux_u64 nux_processor_test_hardware_id(void) { return 3; }

void nux_arch_lock(volatile nux_u32 *lock){while(*lock){}*lock=1;}
void nux_arch_unlock(volatile nux_u32 *lock){*lock=0;}
nux_status nux_thread_set_processor_set(nux_u32 thread,nux_u32 set){(void)thread;(void)set;return NUX_OK;}
nux_u32 nux_thread_processor_set_busy(nux_u32 set){(void)set;return 0;}
nux_status nux_acpi_get_info(nux_acpi_info *out)
{
    if (!out) return NUX_E_INVALID;
    for (nux_u32 i = 0; i < sizeof(*out); ++i) ((nux_u8 *)out)[i] = 0;
    out->cpu_count = 1;
    out->cpus[0].uid = 7;
    out->cpus[0].apic_id = 3;
    out->cpus[0].hardware_id = 3;
    out->cpus[0].flags = 1;
    out->cpus[0].x2apic = 0;
    return NUX_OK;
}

int main(void)
{
    nux_processor_info info;
    if (nux_processor_system_init() != NUX_OK) return 1;
    if (nux_processor_count() != 1) return 2;
    if (nux_processor_get_info(0, &info) != NUX_OK) return 3;
    if (info.id != 0 || !info.online ||
        info.architecture != NUX_PROCESSOR_ARCH_X86_64 ||
        info.logical_id != 7 || info.hardware_id != 3) return 4;
    if (nux_processor_start(0) != NUX_OK) return 5;
    if (nux_processor_stop(0) != NUX_E_ACCESS) return 6;
    if (nux_processor_get_info(1, &info) != NUX_E_NOT_FOUND) return 7;
    nux_processor_set_id set;
    if (nux_processor_set_create(&set) != NUX_OK) return 8;
    if (!nux_processor_set_cpu_allowed(set, 0)) return 9;
    if (nux_processor_set_assign_cpu(set, 0) != NUX_OK) return 10;
    if (nux_processor_set_assign_thread(set, 42) != NUX_OK) return 11;
    if (nux_processor_set_policy(set, NUX_PROCESSOR_POLICY_FIXED_PRIORITY, 12) != NUX_OK) return 12;
    if (nux_processor_set_destroy(set) != NUX_OK) return 13;
    if (nux_processor_set_exists(set)) return 14;
    return 0;
}
