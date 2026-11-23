#ifndef NUX_ACPI_H
#define NUX_ACPI_H

#include <nux/types.h>
#include <nux/status.h>

#define NUX_ACPI_CPU_LIMIT 256u
#define NUX_ACPI_IOAPIC_LIMIT 16u

typedef struct {
    nux_u32 uid;
    nux_u32 apic_id;
    nux_u32 flags;
    nux_u64 hardware_id;
    nux_u8 x2apic;
} nux_acpi_cpu;

typedef struct {
    nux_u8 id;
    nux_u32 address;
    nux_u32 global_irq_base;
} nux_acpi_ioapic;

typedef struct {
    nux_u64 local_apic_address;
    nux_u32 flags;
    nux_u32 cpu_count;
    nux_u32 ioapic_count;
    nux_u64 gic_distributor_address;
    nux_u64 gic_redistributor_address;
    nux_u32 gic_cpu_interface_count;
    nux_acpi_cpu cpus[NUX_ACPI_CPU_LIMIT];
    nux_acpi_ioapic ioapics[NUX_ACPI_IOAPIC_LIMIT];
} nux_acpi_info;

/* Limine base revision 4+ supplies the RSDP as an HHDM virtual pointer. */
nux_status nux_acpi_discover(const void *rsdp_address, nux_acpi_info *out);
nux_status nux_acpi_system_init(const void *rsdp_address);
nux_status nux_acpi_get_info(nux_acpi_info *out);

#endif
