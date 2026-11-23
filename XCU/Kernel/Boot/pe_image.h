#ifndef LENIX_PE_IMAGE_H
#define LENIX_PE_IMAGE_H
#include "uefi.h"
#ifndef LENIX_PE_EXPECT_MACHINE
#if defined(__x86_64__)
#define LENIX_PE_EXPECT_MACHINE 0x8664u
#elif defined(__aarch64__)
#define LENIX_PE_EXPECT_MACHINE 0xaa64u
#else
#error Unsupported EFI boot architecture
#endif
#endif
static efi_u16 lenix_pe_u16(const efi_u8 *p) { return (efi_u16)(p[0] | ((efi_u16)p[1] << 8)); }
static efi_u32 lenix_pe_u32(const efi_u8 *p) {
    return (efi_u32)p[0] | ((efi_u32)p[1] << 8) | ((efi_u32)p[2] << 16) | ((efi_u32)p[3] << 24);
}
static int lenix_pe_image_valid(const efi_u8 *image, efi_u64 size)
{
    efi_u32 pe, image_size, headers_size;
    efi_u16 sections, optional_size;
    efi_u64 table, table_size;
    if (!image || size < 64 || image[0] != 'M' || image[1] != 'Z') return 0;
    pe = lenix_pe_u32(image + 0x3c);
    if ((efi_u64)pe > size || size - pe < 24 || lenix_pe_u32(image + pe) != 0x00004550u ||
        lenix_pe_u16(image + pe + 4) != LENIX_PE_EXPECT_MACHINE) return 0;
    sections = lenix_pe_u16(image + pe + 6);
    optional_size = lenix_pe_u16(image + pe + 20);
    if (!sections || sections > 96 || optional_size < 70 || (efi_u64)pe + 24 + optional_size > size) return 0;
    const efi_u8 *optional = image + pe + 24;
    if (lenix_pe_u16(optional) != 0x20bu || lenix_pe_u16(optional + 68) != 10u) return 0;
    image_size = lenix_pe_u32(optional + 56);
    headers_size = lenix_pe_u32(optional + 60);
    if (!image_size || image_size > EFI_MAX_IMAGE_SIZE || !headers_size || headers_size > size) return 0;
    table = (efi_u64)pe + 24 + optional_size;
    table_size = (efi_u64)sections * 40;
    if (table > size || table_size > size - table || table + table_size > headers_size) return 0;
    for (efi_u16 i = 0; i < sections; ++i) {
        const efi_u8 *section = image + table + (efi_u64)i * 40;
        efi_u32 virtual_size = lenix_pe_u32(section + 8);
        efi_u32 virtual_address = lenix_pe_u32(section + 12);
        efi_u32 raw_size = lenix_pe_u32(section + 16);
        efi_u32 raw_offset = lenix_pe_u32(section + 20);
        if (raw_size && ((efi_u64)raw_offset > size || (efi_u64)raw_size > size - raw_offset)) return 0;
        if (virtual_address > image_size || virtual_size > image_size - virtual_address ||
            raw_size > image_size - virtual_address) return 0;
    }
    return 1;
}
#endif
