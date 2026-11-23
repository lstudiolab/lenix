#include <stdio.h>
#include <string.h>
#include "pe_image.h"
static efi_u8 image[0x300];
static void put16(efi_u8 *p, efi_u16 v) { p[0]=(efi_u8)v; p[1]=(efi_u8)(v>>8); }
static void put32(efi_u8 *p, efi_u32 v) { p[0]=(efi_u8)v; p[1]=(efi_u8)(v>>8); p[2]=(efi_u8)(v>>16); p[3]=(efi_u8)(v>>24); }
static void valid(void) {
    efi_u32 pe=0x80, opt=pe+24, sec=opt+0xf0;
    memset(image,0,sizeof(image)); image[0]='M'; image[1]='Z'; put32(image+0x3c,pe);
    put32(image+pe,0x4550); put16(image+pe+4,LENIX_PE_EXPECT_MACHINE);
    put16(image+pe+6,1); put16(image+pe+20,0xf0); put16(image+opt,0x20b);
    put32(image+opt+56,0x2000); put32(image+opt+60,0x200); put16(image+opt+68,10);
    put32(image+sec+8,0x100); put32(image+sec+12,0x1000);
    put32(image+sec+16,0x100); put32(image+sec+20,0x200);
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failure at %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void) {
    valid(); CHECK(lenix_pe_image_valid(image,sizeof(image)));
    image[0]=0; CHECK(!lenix_pe_image_valid(image,sizeof(image))); valid();
    put16(image+0x84,LENIX_PE_EXPECT_MACHINE==0x8664u?0xaa64u:0x8664u);
    CHECK(!lenix_pe_image_valid(image,sizeof(image))); valid();
    put16(image+0x98,0x10b); CHECK(!lenix_pe_image_valid(image,sizeof(image))); valid();
    put16(image+0x98+68,3); CHECK(!lenix_pe_image_valid(image,sizeof(image))); valid();
    put32(image+0x80+24+0xf0+20,0xfffffff0u);
    CHECK(!lenix_pe_image_valid(image,sizeof(image))); valid();
    put32(image+0x80+24+0xf0+12,0x1ff0u);
    CHECK(!lenix_pe_image_valid(image,sizeof(image))); CHECK(!lenix_pe_image_valid(image,63));
    puts("PE validation passed"); return 0;
}
