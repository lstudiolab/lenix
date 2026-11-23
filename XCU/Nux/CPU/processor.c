#include <nux/processor.h>
#include <nux/acpi.h>

static nux_processor_info processors[NUX_CPU_LIMIT];
static nux_u32 processor_total;
static nux_cpu_id boot_cpu;

static nux_u64 nux_arch_processor_hardware_id(void)
{
#if defined(__x86_64__)
    nux_u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0), "c"(0));
    nux_u32 maximum = eax;
    if (maximum >= 0x0b) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(0x0b), "c"(0));
        if (ebx) return edx;
    }
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1), "c"(0));
    return (ebx >> 24) & 0xffu;
#elif defined(__aarch64__)
    nux_u64 id;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(id));
    return id & 0xff00ffffffULL;
#else
#error Unsupported Nux architecture
#endif
}

nux_status nux_processor_system_init(void)
{
    nux_acpi_info topology;
    nux_u64 current_id = nux_arch_processor_hardware_id();
    processor_total = 0;
    boot_cpu = 0;

    for (nux_u32 i = 0; i < NUX_CPU_LIMIT; ++i) {
        processors[i].id = i;
        processors[i].architecture =
#if defined(__x86_64__)
            NUX_PROCESSOR_ARCH_X86_64;
#else
            NUX_PROCESSOR_ARCH_ARM64;
#endif
        processors[i].logical_id = i;
        processors[i].numa_node = 0;
        processors[i].features = 0;
        processors[i].frequency_hz = 0;
        processors[i].online = 0;
    }

    if (nux_acpi_get_info(&topology) == NUX_OK) {
        for (nux_u32 i = 0; i < topology.cpu_count &&
             processor_total < NUX_CPU_LIMIT; ++i) {
            nux_acpi_cpu *source = &topology.cpus[i];
            if (!(source->flags & 3u)) continue;
#if defined(__x86_64__)
            if (source->x2apic == 2) continue;
            nux_u64 hardware_id = source->hardware_id;
#else
            if (source->x2apic != 2) continue;
            nux_u64 hardware_id = source->hardware_id & 0xff00ffffffULL;
#endif
            nux_processor_info *target = &processors[processor_total];
            target->id = processor_total;
            target->logical_id = source->uid;
            target->features = hardware_id;
            target->online = hardware_id == current_id;
            if (target->online) boot_cpu = processor_total;
            ++processor_total;
        }
    }

    if (!processor_total) {
        processors[0].features = current_id;
        processors[0].online = 1;
        processor_total = 1;
        boot_cpu = 0;
    } else {
        nux_u32 online_found = 0;
        for (nux_u32 i = 0; i < processor_total; ++i)
            online_found |= processors[i].online;
        if (!online_found) {
            processors[0].online = 1;
            boot_cpu = 0;
        }
    }
    return NUX_OK;
}

nux_u32 nux_processor_count(void) { return processor_total; }

nux_status nux_processor_get_info(nux_cpu_id cpu, nux_processor_info *out)
{
    if (!out) return NUX_E_INVALID;
    if (cpu >= processor_total) return NUX_E_NOT_FOUND;
    *out = processors[cpu];
    return NUX_OK;
}

nux_status nux_processor_start(nux_cpu_id cpu)
{
    if (cpu >= processor_total) return NUX_E_NOT_FOUND;
    return processors[cpu].online ? NUX_OK : NUX_E_NOT_SUPPORTED;
}

nux_status nux_processor_stop(nux_cpu_id cpu)
{
    if (cpu >= processor_total) return NUX_E_NOT_FOUND;
    if (cpu == boot_cpu) return NUX_E_ACCESS;
    return processors[cpu].online ? NUX_E_NOT_SUPPORTED : NUX_OK;
}
