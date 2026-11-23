#include <nux/boot.h>

#define TEST_MAGIC 0x4e55580000000000ULL

int main(void)
{
    nux_boot_mark(NUX_BOOT_PHASE_ENTRY);
    if (nux_boot_status != (TEST_MAGIC | ((nux_u64)NUX_BOOT_PHASE_ENTRY << 32)))
        return 1;
    nux_boot_mark(NUX_BOOT_PHASE_VIRTUAL_MEMORY);
    if (nux_boot_status != (TEST_MAGIC | ((nux_u64)NUX_BOOT_PHASE_VIRTUAL_MEMORY << 32)))
        return 2;
    nux_boot_mark(NUX_BOOT_PHASE_READY);
    if (nux_boot_status != (TEST_MAGIC | ((nux_u64)NUX_BOOT_PHASE_READY << 32)))
        return 3;
    return 0;
}
