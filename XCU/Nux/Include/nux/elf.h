#ifndef NUX_ELF_H
#define NUX_ELF_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/task.h>

#define NUX_ELF_STACK_SIZE (64ULL * 1024ULL)
#define NUX_ELF_STACK_TOP 0x00007fff00000000ULL

typedef struct {
    nux_u64 entry;
    nux_u64 stack_base;
    nux_u64 stack_pointer;
    nux_u64 stack_size;
    nux_u32 segment_count;
    nux_u32 reserved;
} nux_elf_load_result;

/* Loads a static ELF64 executable and a zeroed user stack into a task address
 * space. Architecture-specific user-mode entry remains a separate operation. */
nux_status nux_elf_load(nux_task_id task, const void *image, nux_size image_size,
                        nux_elf_load_result *result);
#endif
