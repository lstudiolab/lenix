#include <nux/processor.h>
#include <nux/acpi.h>

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
    return 0;
}
