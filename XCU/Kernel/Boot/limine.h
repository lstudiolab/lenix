#ifndef NUX_LIMINE_H
#define NUX_LIMINE_H

#include <nux/types.h>

/* Minimal Limine protocol declarations; kept local to avoid vendored code. */
#define LIMINE_COMMON_MAGIC0 0xc7b1dd30df4c8b88ULL
#define LIMINE_COMMON_MAGIC1 0x0a82e883a194f07bULL
#define LIMINE_BASE_REVISION { 0xf9562b2d5c95a6c8ULL, 0x6a7b384944536bdcULL, 6ULL }
#define LIMINE_MEMMAP_REQUEST_ID { LIMINE_COMMON_MAGIC0, LIMINE_COMMON_MAGIC1, 0x67cf3d9d378a806fULL, 0xe304acdfc50c3c62ULL }
#define LIMINE_HHDM_REQUEST_ID { LIMINE_COMMON_MAGIC0, LIMINE_COMMON_MAGIC1, 0x48dcf1cb8ad2b852ULL, 0x63984e959a98244bULL }
#define LIMINE_RSDP_REQUEST_ID { LIMINE_COMMON_MAGIC0, LIMINE_COMMON_MAGIC1, 0xc5e77b6b397e7b43ULL, 0x27637845accdcf3cULL }

typedef struct limine_base_revision {
    nux_u64 magic0;
    nux_u64 magic1;
    nux_u64 revision;
} limine_base_revision;

typedef struct limine_request {
    nux_u64 id[4];
    nux_u64 revision;
    void *response;
} limine_request;

typedef struct limine_memmap_entry {
    nux_u64 base;
    nux_u64 length;
    nux_u64 type;
} limine_memmap_entry;

typedef struct limine_memmap_response {
    nux_u64 revision;
    nux_u64 entry_count;
    limine_memmap_entry **entries;
} limine_memmap_response;

typedef struct limine_rsdp_response {
    nux_u64 revision;
    void *address;
} limine_rsdp_response;

typedef struct limine_hhdm_response {
    nux_u64 revision;
    nux_u64 offset;
} limine_hhdm_response;

extern limine_base_revision limine_base_revision_tag;
extern limine_request limine_memmap_request;
extern limine_request limine_hhdm_request;
extern limine_request limine_rsdp_request;

#endif
