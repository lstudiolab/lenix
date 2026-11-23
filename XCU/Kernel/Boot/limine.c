#include <nux/types.h>
#include "limine.h"

/* Delimit the protocol request region so discovery is deterministic. */
__attribute__((used, aligned(8), section(".limine_requests_start")))
const nux_u64 limine_requests_start_marker[4] = {
    0xf6b8f4b39de7d1aeULL, 0xfab91a6940fcb9cfULL,
    0x785c6ed015d3e316ULL, 0x181e920a7852b9d9ULL
};

__attribute__((used, aligned(8), section(".limine_requests")))
limine_base_revision limine_base_revision_tag = LIMINE_BASE_REVISION;

__attribute__((used, aligned(8), section(".limine_requests")))
limine_request limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0, .response = 0
};

__attribute__((used, aligned(8), section(".limine_requests")))
limine_request limine_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID, .revision = 0, .response = 0
};

__attribute__((used, aligned(8), section(".limine_requests_end")))
const nux_u64 limine_requests_end_marker[2] = {
    0xadc0e0531bb10d03ULL, 0x9572709f31764c62ULL
};
