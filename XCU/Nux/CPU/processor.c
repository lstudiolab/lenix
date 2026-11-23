#include <nux/processor.h>
#include <nux/acpi.h>
#include <nux/thread.h>

#define NUX_PROCESSOR_SET_WIDTH ((NUX_CPU_LIMIT + 63u) / 64u)

typedef struct {
    nux_processor_set_id id;
    nux_u64 cpus[NUX_PROCESSOR_SET_WIDTH];
    nux_u32 policy;
    nux_u8 priority;
    nux_u8 active;
} nux_processor_set_entry;

static nux_processor_info processors[NUX_CPU_LIMIT];
static nux_processor_set_entry processor_sets[NUX_PROCESSOR_SET_LIMIT];
static nux_u32 processor_total;
static nux_cpu_id boot_cpu;
static nux_processor_set_id next_set_id;
static volatile nux_u32 processor_lock;

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

static nux_processor_set_entry *processor_set_find(nux_processor_set_id id)
{
    for (nux_u32 i = 0; i < NUX_PROCESSOR_SET_LIMIT; ++i)
        if (processor_sets[i].active == 1 && processor_sets[i].id == id)
            return &processor_sets[i];
    return NUX_NULL;
}

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
    next_set_id = 1;
    processor_lock = 0;
    for (nux_u32 i = 0; i < NUX_PROCESSOR_SET_LIMIT; ++i)
        processor_sets[i].active = 0;

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
        processors[i].hardware_id = 0;
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
            target->hardware_id = hardware_id;
            target->online = hardware_id == current_id;
            if (target->online) boot_cpu = processor_total;
            ++processor_total;
        }
    }

    if (!processor_total) {
        processors[0].hardware_id = current_id;
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

nux_u32 nux_processor_set_exists(nux_processor_set_id id)
{
    nux_u32 exists;
    nux_arch_lock(&processor_lock);
    exists = processor_set_find(id) != NUX_NULL;
    nux_arch_unlock(&processor_lock);
    return exists;
}

nux_u32 nux_processor_set_cpu_allowed(nux_processor_set_id id, nux_cpu_id cpu)
{
    if (!id) return 1;
    if (cpu >= processor_total) return 0;
    nux_arch_lock(&processor_lock);
    nux_processor_set_entry *set = processor_set_find(id);
    nux_u32 allowed = set &&
        ((set->cpus[cpu >> 6] >> (cpu & 63u)) & 1u);
    nux_arch_unlock(&processor_lock);
    return allowed;
}

nux_cpu_id nux_processor_current_id(void) { return boot_cpu; }

nux_status nux_processor_set_create(nux_processor_set_id *out)
{
    if (!out) return NUX_E_INVALID;
    nux_arch_lock(&processor_lock);
    for (nux_u32 i = 0; i < NUX_PROCESSOR_SET_LIMIT; ++i) {
        nux_processor_set_entry *set = &processor_sets[i];
        if (set->active) continue;
        set->id = next_set_id++;
        if (!set->id) set->id = next_set_id++;
        for (nux_u32 j = 0; j < NUX_PROCESSOR_SET_WIDTH; ++j)
            set->cpus[j] = 0;
        for (nux_u32 j = 0; j < processor_total; ++j)
            if (processors[j].online)
                set->cpus[j >> 6] |= (nux_u64)1 << (j & 63u);
        set->policy = NUX_PROCESSOR_POLICY_TIMESHARE;
        set->priority = 0;
        set->active = 1;
        *out = set->id;
        nux_arch_unlock(&processor_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&processor_lock);
    return NUX_E_LIMIT;
}

nux_status nux_processor_set_destroy(nux_processor_set_id id)
{
    if (!id) return NUX_E_INVALID;
    nux_arch_lock(&processor_lock);
    nux_processor_set_entry *set = processor_set_find(id);
    if (!set) {
        nux_arch_unlock(&processor_lock);
        return NUX_E_NOT_FOUND;
    }
    set->active = 2;
    nux_arch_unlock(&processor_lock);

    if (nux_thread_processor_set_busy(id)) {
        nux_arch_lock(&processor_lock);
        for (nux_u32 i = 0; i < NUX_PROCESSOR_SET_LIMIT; ++i)
            if (processor_sets[i].id == id && processor_sets[i].active == 2)
                processor_sets[i].active = 1;
        nux_arch_unlock(&processor_lock);
        return NUX_E_BUSY;
    }

    nux_arch_lock(&processor_lock);
    for (nux_u32 i = 0; i < NUX_PROCESSOR_SET_LIMIT; ++i) {
        if (processor_sets[i].id != id || processor_sets[i].active != 2)
            continue;
        processor_sets[i].active = 0;
        break;
    }
    nux_arch_unlock(&processor_lock);
    return NUX_OK;
}

nux_status nux_processor_set_assign_cpu(nux_processor_set_id id, nux_cpu_id cpu)
{
    if (!id || cpu >= processor_total) return NUX_E_INVALID;
    nux_arch_lock(&processor_lock);
    nux_processor_set_entry *set = processor_set_find(id);
    if (!set) {
        nux_arch_unlock(&processor_lock);
        return NUX_E_NOT_FOUND;
    }
    set->cpus[cpu >> 6] |= (nux_u64)1 << (cpu & 63u);
    nux_arch_unlock(&processor_lock);
    return NUX_OK;
}

nux_status nux_processor_set_assign_thread(nux_processor_set_id id,
                                           nux_u32 thread_id)
{
    if (!nux_processor_set_exists(id)) return NUX_E_NOT_FOUND;
    return nux_thread_set_processor_set(thread_id, id);
}

nux_status nux_processor_set_policy(nux_processor_set_id id, nux_u32 policy,
                                    nux_u8 priority)
{
    if (!id || policy > NUX_PROCESSOR_POLICY_REALTIME || priority > 127)
        return NUX_E_INVALID;
    nux_arch_lock(&processor_lock);
    nux_processor_set_entry *set = processor_set_find(id);
    if (!set) {
        nux_arch_unlock(&processor_lock);
        return NUX_E_NOT_FOUND;
    }
    set->policy = policy;
    set->priority = priority;
    nux_arch_unlock(&processor_lock);
    return NUX_OK;
}

nux_status nux_thread_set_affinity(nux_u32 thread_id,
                                   nux_processor_set_id set)
{
    return nux_thread_set_processor_set(thread_id, set);
}
