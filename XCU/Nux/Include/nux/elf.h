#ifndef NUX_ELF_H
#define NUX_ELF_H
#include <nux/types.h>
#include <nux/status.h>
typedef struct {
    nux_u64 entry;
    nux_u32 segment_count;
    nux_u32 reserved;
} nux_elf_load_result;
/* Loads a static ELF64 executable into a task address space. User-mode entry
 * and stack setup are separate architecture-thread operations. */
nux_status nux_elf_load(nux_task_id task, const void *image, nux_size image_size,
                        nux_elf_load_result *result);
#endif
