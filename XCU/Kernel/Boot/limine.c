#include <nux/types.h>
#include "limine.h"

__attribute__((used, section(".limine_requests")))
limine_base_revision limine_base_revision_tag = LIMINE_BASE_REVISION;

__attribute__((used, section(".limine_requests")))
limine_request limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0, .response = 0
};

__attribute__((used, section(".limine_requests")))
limine_request limine_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID, .revision = 0, .response = 0
};
