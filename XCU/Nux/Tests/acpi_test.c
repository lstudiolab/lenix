#include <nux/acpi.h>
#include <nux/memory.h>

static nux_u8 image[4096];

void *nux_phys_to_virt(nux_u64 physical)
{
    if (physical >= sizeof(image)) return NUX_NULL;
    return image + physical;
}

static void put32(nux_u8 *p, nux_u32 value)
{
    for (nux_u32 i = 0; i < 4; ++i) p[i] = (nux_u8)(value >> (i * 8));
}

static void put64(nux_u8 *p, nux_u64 value)
{
    for (nux_u32 i = 0; i < 8; ++i) p[i] = (nux_u8)(value >> (i * 8));
}

static void set_checksum(nux_u8 *bytes, nux_u32 length, nux_u32 offset)
{
    nux_u8 sum = 0;
    bytes[offset] = 0;
    for (nux_u32 i = 0; i < length; ++i) sum = (nux_u8)(sum + bytes[i]);
    bytes[offset] = (nux_u8)(0u - sum);
}

int main(void)
{
    const nux_u32 madt_length = 44 + 8 + 12 + 80 + 24 + 16;
    nux_u8 *rsdp = image + 0x100;
    nux_u8 *xsdt = image + 0x200;
    nux_u8 *madt = image + 0x300;
    nux_acpi_info info;

    for (nux_u32 i = 0; i < 8; ++i) rsdp[i] = (nux_u8)"RSD PTR "[i];
    rsdp[15] = 2;
    put32(rsdp + 20, 36);
    put64(rsdp + 24, 0x200);
    set_checksum(rsdp, 20, 8);
    set_checksum(rsdp, 36, 32);

    for (nux_u32 i = 0; i < 4; ++i) xsdt[i] = (nux_u8)"XSDT"[i];
    put32(xsdt + 4, 44);
    xsdt[8] = 1;
    put64(xsdt + 36, 0x300);
    set_checksum(xsdt, 44, 9);

    for (nux_u32 i = 0; i < 4; ++i) madt[i] = (nux_u8)"APIC"[i];
    put32(madt + 4, madt_length);
    madt[8] = 5;
    put32(madt + 36, 0xfee00000);
    put32(madt + 40, 1);

    /* x86 Local APIC processor and I/O APIC. */
    madt[44] = 0; madt[45] = 8; madt[46] = 0; madt[47] = 2;
    put32(madt + 48, 1);
    madt[52] = 1; madt[53] = 12; madt[54] = 3;
    put32(madt + 56, 0xfec00000);
    put32(madt + 60, 0);

    /* ARM GIC CPU interface, distributor, and redistributor. */
    madt[64] = 11; madt[65] = 80;
    put32(madt + 72, 7);
    put32(madt + 76, 1);
    put64(madt + 132, 0x80000000);
    madt[144] = 12; madt[145] = 24;
    put64(madt + 152, 0x08000000);
    madt[168] = 14; madt[169] = 16;
    put64(madt + 172, 0x080a0000);
    set_checksum(madt, madt_length, 9);

    if (madt_length > sizeof(image) - 0x300) return 1;
    if (nux_acpi_discover(rsdp, &info) != NUX_OK) return 2;
    if (info.local_apic_address != 0xfee00000 || info.cpu_count != 2 ||
        info.cpus[0].apic_id != 2 || info.ioapic_count != 1 ||
        info.ioapics[0].address != 0xfec00000) return 3;
    if (info.gic_cpu_interface_count != 1 ||
        info.gic_distributor_address != 0x08000000 ||
        info.gic_redistributor_address != 0x080a0000) return 4;

    rsdp[8]++;
    if (nux_acpi_discover(rsdp, &info) != NUX_E_BAD_STATE) return 5;
    if (nux_acpi_discover(NUX_NULL, &info) != NUX_E_INVALID) return 6;
    return 0;
}
