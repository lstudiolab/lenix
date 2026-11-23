#include <nux/acpi.h>
#include <nux/memory.h>

#define NUX_ACPI_TABLE_MAX (1024u * 1024u)
#define NUX_ACPI_RSDP_MAX 4096u

typedef struct __attribute__((packed)) {
    char signature[8];
    nux_u8 checksum;
    char oem_id[6];
    nux_u8 revision;
    nux_u32 rsdt_address;
    nux_u32 length;
    nux_u64 xsdt_address;
    nux_u8 extended_checksum;
    nux_u8 reserved[3];
} nux_acpi_rsdp;

typedef struct __attribute__((packed)) {
    char signature[4];
    nux_u32 length;
    nux_u8 revision;
    nux_u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    nux_u32 oem_revision;
    nux_u32 creator_id;
    nux_u32 creator_revision;
} nux_acpi_header;

typedef struct __attribute__((packed)) {
    nux_acpi_header header;
    nux_u32 local_apic_address;
    nux_u32 flags;
} nux_acpi_madt;

typedef struct __attribute__((packed)) {
    nux_u8 type;
    nux_u8 length;
} nux_acpi_record;

static nux_u32 checksum_valid(const nux_u8 *bytes, nux_u32 length)
{
    nux_u8 sum = 0;
    for (nux_u32 i = 0; i < length; ++i) sum = (nux_u8)(sum + bytes[i]);
    return sum == 0;
}

static nux_u32 signature_equal(const char *left, const char *right, nux_u32 n)
{
    for (nux_u32 i = 0; i < n; ++i)
        if (left[i] != right[i]) return 0;
    return 1;
}

static const void *physical_table(nux_u64 physical, nux_u32 minimum,
                                  nux_acpi_header **header_out)
{
    nux_acpi_header *header = (nux_acpi_header *)nux_phys_to_virt(physical);
    if (!header || header->length < minimum ||
        header->length > NUX_ACPI_TABLE_MAX ||
        physical > NUX_U64_MAX - header->length)
        return NUX_NULL;
    if (!checksum_valid((const nux_u8 *)header, header->length))
        return NUX_NULL;
    if (header_out) *header_out = header;
    return header;
}

static nux_status parse_madt(nux_u64 physical, nux_acpi_info *out)
{
    nux_acpi_header *header;
    const nux_acpi_madt *madt = (const nux_acpi_madt *)
        physical_table(physical, sizeof(nux_acpi_madt), &header);
    if (!madt || !signature_equal(header->signature, "APIC", 4))
        return NUX_E_BAD_STATE;

    out->local_apic_address = madt->local_apic_address;
    out->flags = madt->flags;
    const nux_u8 *cursor = (const nux_u8 *)madt + sizeof(*madt);
    const nux_u8 *end = (const nux_u8 *)madt + header->length;
    while (cursor < end) {
        const nux_acpi_record *record = (const nux_acpi_record *)cursor;
        if ((nux_u64)(end - cursor) < sizeof(*record) ||
            record->length < sizeof(*record) ||
            (nux_u64)(end - cursor) < record->length)
            return NUX_E_BAD_STATE;

        if (record->type == 0 && record->length >= 8) {
            if (out->cpu_count < NUX_ACPI_CPU_LIMIT) {
                const nux_u8 *r = cursor;
                nux_acpi_cpu *cpu = &out->cpus[out->cpu_count++];
                cpu->uid = r[2];
                cpu->apic_id = r[3];
                cpu->hardware_id = r[3];
                cpu->flags = (nux_u32)r[4] | ((nux_u32)r[5] << 8) |
                    ((nux_u32)r[6] << 16) | ((nux_u32)r[7] << 24);
                cpu->x2apic = 0;
            }
        } else if (record->type == 1 && record->length >= 12) {
            if (out->ioapic_count < NUX_ACPI_IOAPIC_LIMIT) {
                const nux_u8 *r = cursor;
                nux_acpi_ioapic *io = &out->ioapics[out->ioapic_count++];
                io->id = r[2];
                io->address = (nux_u32)r[4] | ((nux_u32)r[5] << 8) |
                    ((nux_u32)r[6] << 16) | ((nux_u32)r[7] << 24);
                io->global_irq_base = (nux_u32)r[8] |
                    ((nux_u32)r[9] << 8) | ((nux_u32)r[10] << 16) |
                    ((nux_u32)r[11] << 24);
            }
        } else if (record->type == 5 && record->length >= 12) {
            const nux_u8 *r = cursor;
            nux_u64 address = 0;
            for (nux_u32 i = 0; i < 8; ++i)
                address |= (nux_u64)r[4 + i] << (i * 8);
            out->local_apic_address = address;
        } else if (record->type == 9 && record->length >= 16) {
            if (out->cpu_count < NUX_ACPI_CPU_LIMIT) {
                const nux_u8 *r = cursor;
                nux_acpi_cpu *cpu = &out->cpus[out->cpu_count++];
                cpu->uid = (nux_u32)r[12] | ((nux_u32)r[13] << 8) |
                    ((nux_u32)r[14] << 16) | ((nux_u32)r[15] << 24);
                cpu->apic_id = (nux_u32)r[4] | ((nux_u32)r[5] << 8) |
                    ((nux_u32)r[6] << 16) | ((nux_u32)r[7] << 24);
                cpu->hardware_id = cpu->apic_id;
                cpu->flags = (nux_u32)r[8] | ((nux_u32)r[9] << 8) |
                    ((nux_u32)r[10] << 16) | ((nux_u32)r[11] << 24);
                cpu->x2apic = 1;
            }
        } else if (record->type == 11 && record->length >= 76) {
            if (out->cpu_count < NUX_ACPI_CPU_LIMIT) {
                const nux_u8 *r = cursor;
                nux_acpi_cpu *cpu = &out->cpus[out->cpu_count++];
                cpu->uid = (nux_u32)r[8] | ((nux_u32)r[9] << 8) |
                    ((nux_u32)r[10] << 16) | ((nux_u32)r[11] << 24);
                cpu->flags = (nux_u32)r[12] | ((nux_u32)r[13] << 8) |
                    ((nux_u32)r[14] << 16) | ((nux_u32)r[15] << 24);
                for (nux_u32 i = 0; i < 8; ++i)
                    cpu->hardware_id |= (nux_u64)r[68 + i] << (i * 8);
                cpu->apic_id = 0;
                cpu->x2apic = 2;
            }
        }
        cursor += record->length;
    }
    return out->local_apic_address ? NUX_OK : NUX_E_BAD_STATE;
}

nux_status nux_acpi_discover(const void *rsdp_address, nux_acpi_info *out)
{
    const nux_acpi_rsdp *rsdp;
    nux_u64 root_physical;
    nux_acpi_header *root;
    nux_u32 entry_size;
    nux_u32 count;

    if (!rsdp_address || !out) return NUX_E_INVALID;
    rsdp = (const nux_acpi_rsdp *)rsdp_address;
    if (!rsdp || !signature_equal(rsdp->signature, "RSD PTR ", 8) ||
        !checksum_valid((const nux_u8 *)rsdp, 20))
        return NUX_E_BAD_STATE;

    for (nux_u32 i = 0; i < sizeof(*out); ++i)
        ((nux_u8 *)out)[i] = 0;

    if (rsdp->revision >= 2) {
        if (rsdp->length < 36 || rsdp->length > NUX_ACPI_RSDP_MAX ||
            !checksum_valid((const nux_u8 *)rsdp, rsdp->length))
            return NUX_E_BAD_STATE;
        root_physical = rsdp->xsdt_address;
        entry_size = 8;
    } else {
        root_physical = rsdp->rsdt_address;
        entry_size = 4;
    }
    if (!root_physical) return NUX_E_NOT_FOUND;

    if (!physical_table(root_physical, sizeof(nux_acpi_header), &root))
        return NUX_E_BAD_STATE;
    if (entry_size == 8) {
        if (!signature_equal(root->signature, "XSDT", 4)) return NUX_E_BAD_STATE;
    } else if (!signature_equal(root->signature, "RSDT", 4)) {
        return NUX_E_BAD_STATE;
    }
    if ((root->length - sizeof(*root)) % entry_size) return NUX_E_BAD_STATE;
    count = (root->length - (nux_u32)sizeof(*root)) / entry_size;
    const nux_u8 *entries = (const nux_u8 *)root + sizeof(*root);
    for (nux_u32 i = 0; i < count; ++i) {
        nux_u64 table_physical = 0;
        for (nux_u32 b = 0; b < entry_size; ++b)
            table_physical |= (nux_u64)entries[i * entry_size + b] << (b * 8);
        if (!table_physical) continue;
        nux_acpi_header *candidate;
        if (!physical_table(table_physical, sizeof(nux_acpi_header), &candidate))
            continue;
        if (signature_equal(candidate->signature, "APIC", 4))
            return parse_madt(table_physical, out);
    }
    return NUX_E_NOT_FOUND;
}

static nux_acpi_info platform_info;
static nux_u32 platform_info_valid;

nux_status nux_acpi_system_init(const void *rsdp_address)
{
    nux_acpi_info discovered;
    if (!rsdp_address) {
        platform_info_valid = 0;
        return NUX_E_NOT_FOUND;
    }
    nux_status status = nux_acpi_discover(rsdp_address, &discovered);
    if (status != NUX_OK) {
        platform_info_valid = 0;
        return status;
    }
    platform_info = discovered;
    platform_info_valid = 1;
    return NUX_OK;
}

nux_status nux_acpi_get_info(nux_acpi_info *out)
{
    if (!out) return NUX_E_INVALID;
    if (!platform_info_valid) return NUX_E_NOT_FOUND;
    *out = platform_info;
    return NUX_OK;
}
