#include <nux/elf.h>
#include <nux/task.h>
#include <nux/vm.h>

#define ELF_HEADER_SIZE 64u
#define ELF_PROGRAM_SIZE 56u
#define ELF_PROGRAM_LIMIT 32u
#define ELF_USER_STACK_SIZE (64u * 1024u)
#define ELF_USER_END 0x00007fff00000000ULL
#define ELF_PT_LOAD 1u
#define ELF_PT_DYNAMIC 2u
#define ELF_PT_INTERP 3u
#define ELF_PF_X 1u
#define ELF_PF_W 2u
#define ELF_PF_R 4u
#define ELF_ET_EXEC 2u
#define ELF_CLASS_64 2u
#define ELF_DATA_LSB 1u
#define ELF_VERSION 1u
#define ELF_EM_X86_64 62u
#define ELF_EM_AARCH64 183u

typedef struct __attribute__((packed)) {
    nux_u8 ident[16];
    nux_u16 type, machine;
    nux_u32 version;
    nux_u64 entry, phoff, shoff;
    nux_u32 flags;
    nux_u16 ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} elf64_header;

typedef struct __attribute__((packed)) {
    nux_u32 type, flags;
    nux_u64 offset, vaddr, paddr, filesz, memsz, align;
} elf64_program;

typedef struct {
    nux_u64 address, size;
    nux_u32 protection;
} loaded_segment;

static nux_u32 supported_machine(void)
{
#if defined(NUX_ELF_TEST_X86_64) || (!defined(NUX_ELF_TEST_ARM64) && defined(__x86_64__))
    return ELF_EM_X86_64;
#elif defined(NUX_ELF_TEST_ARM64) || defined(__aarch64__)
    return ELF_EM_AARCH64;
#else
#error Unsupported Nux ELF target
#endif
}

static nux_u64 align_down_page(nux_u64 value)
{
    return value & ~(NUX_VM_PAGE_SIZE - 1);
}

static nux_u64 align_up_page(nux_u64 value)
{
    if (value > NUX_U64_MAX - (NUX_VM_PAGE_SIZE - 1)) return 0;
    return (value + NUX_VM_PAGE_SIZE - 1) & ~(NUX_VM_PAGE_SIZE - 1);
}

nux_status nux_elf_load(nux_task_id task, const void *image,
                        nux_size image_size, nux_elf_load_result *result)
{
    const nux_u8 *bytes = (const nux_u8 *)image;
    const elf64_header *header;
    loaded_segment loaded[ELF_PROGRAM_LIMIT];
    nux_u32 loaded_count = 0;
    nux_u64 address_space = 0;
    nux_u32 executable_entry = 0;
    nux_u32 stack_mapped = 0;
    nux_u64 stack_base = ELF_USER_END - ELF_USER_STACK_SIZE;
    nux_status failure_status = NUX_E_BAD_STATE;

    if (!task || !image || !result || image_size < ELF_HEADER_SIZE)
        return NUX_E_INVALID;
    result->entry = 0;
    result->segment_count = 0;
    result->reserved = 0;
    result->stack_base = 0;
    result->stack_pointer = 0;
    result->stack_size = 0;
    header = (const elf64_header *)bytes;
    if (bytes[0] != 0x7f || bytes[1] != 'E' || bytes[2] != 'L' ||
        bytes[3] != 'F' || header->ident[4] != ELF_CLASS_64 ||
        header->ident[5] != ELF_DATA_LSB || header->ident[6] != ELF_VERSION ||
        header->type != ELF_ET_EXEC || header->machine != supported_machine() ||
        header->version != ELF_VERSION || header->ehsize != ELF_HEADER_SIZE ||
        header->phentsize != ELF_PROGRAM_SIZE || !header->phnum ||
        header->phnum > ELF_PROGRAM_LIMIT)
        return NUX_E_NOT_SUPPORTED;
    if (header->phoff > image_size ||
        (nux_u64)header->phnum * header->phentsize > image_size - header->phoff)
        return NUX_E_BAD_STATE;
    if (nux_task_get_address_space(task, &address_space) != NUX_OK ||
        !address_space)
        return NUX_E_BAD_STATE;

    for (nux_u32 i = 0; i < header->phnum; ++i) {
        const elf64_program *program = (const elf64_program *)
            (bytes + header->phoff + (nux_u64)i * header->phentsize);
        if (program->type == ELF_PT_DYNAMIC || program->type == ELF_PT_INTERP) {
            failure_status = NUX_E_NOT_SUPPORTED;
            goto load_failed;
        }
        if (program->type != ELF_PT_LOAD || !program->memsz) continue;
        if (program->memsz < program->filesz ||
            program->offset > image_size ||
            program->filesz > image_size - program->offset ||
            program->vaddr > NUX_U64_MAX - program->memsz ||
            (program->offset & (NUX_VM_PAGE_SIZE - 1)) !=
                (program->vaddr & (NUX_VM_PAGE_SIZE - 1)) ||
            ((program->flags & ELF_PF_W) && (program->flags & ELF_PF_X)) ||
            (program->flags & ~(ELF_PF_R | ELF_PF_W | ELF_PF_X)) ||
            !(program->flags & ELF_PF_R) ||
            (program->align > 1 &&
             (program->align & (program->align - 1))) ||
            (program->align > 1 &&
             (program->vaddr % program->align) !=
                (program->offset % program->align)))
            goto invalid_image;

        nux_u64 start = align_down_page(program->vaddr);
        nux_u64 end = align_up_page(program->vaddr + program->memsz);
        if (!end || end <= start) goto invalid_image;
        nux_u32 protection = NUX_VM_PROT_READ | NUX_VM_PROT_WRITE;
        if (loaded_count == ELF_PROGRAM_LIMIT) goto invalid_image;

        nux_vm_address mapped = start;
        nux_status status = nux_vm_map(address_space, &mapped, end - start,
                                       protection, 0, 0);
        if (status != NUX_OK) { failure_status = status; goto load_failed; }
        loaded[loaded_count].address = start;
        loaded[loaded_count].size = end - start;
        loaded[loaded_count].protection =
            ((program->flags & ELF_PF_R) ? NUX_VM_PROT_READ : 0) |
            ((program->flags & ELF_PF_W) ? NUX_VM_PROT_WRITE : 0) |
            ((program->flags & ELF_PF_X) ? NUX_VM_PROT_EXEC : 0);
        ++loaded_count;

        if (program->filesz) {
            status = nux_vm_write(address_space, program->vaddr,
                                  bytes + program->offset,
                                  (nux_size)program->filesz);
            if (status != NUX_OK) { failure_status = status; goto load_failed; }
        }
        if ((program->flags & ELF_PF_X) && header->entry >= program->vaddr &&
            header->entry - program->vaddr < program->memsz)
            executable_entry = 1;
    }
    if (!loaded_count || !executable_entry) goto invalid_image;
    nux_vm_address stack_address = stack_base;
    nux_status stack_status = nux_vm_map(address_space, &stack_address,
        ELF_USER_STACK_SIZE, NUX_VM_PROT_READ | NUX_VM_PROT_WRITE, 0, 0);
    if (stack_status != NUX_OK) {
        failure_status = stack_status;
        goto load_failed;
    }
    stack_mapped = 1;
    for (nux_u32 i = 0; i < loaded_count; ++i) {
        nux_status status = nux_vm_protect(address_space, loaded[i].address,
                                           loaded[i].size,
                                           loaded[i].protection);
        if (status != NUX_OK) { failure_status = status; goto load_failed; }
    }
    result->entry = header->entry;
    result->segment_count = loaded_count;
    result->reserved = 0;
    result->stack_base = stack_base;
    result->stack_pointer = (stack_base + ELF_USER_STACK_SIZE) & ~0xfULL;
    result->stack_size = ELF_USER_STACK_SIZE;
    return NUX_OK;

invalid_image:
    if (stack_mapped)
        (void)nux_vm_unmap(address_space, stack_base, ELF_USER_STACK_SIZE);
    while (loaded_count) {
        --loaded_count;
        (void)nux_vm_unmap(address_space, loaded[loaded_count].address,
                           loaded[loaded_count].size);
    }
    return NUX_E_BAD_STATE;

load_failed:
    if (stack_mapped)
        (void)nux_vm_unmap(address_space, stack_base, ELF_USER_STACK_SIZE);
    while (loaded_count) {
        --loaded_count;
        (void)nux_vm_unmap(address_space, loaded[loaded_count].address,
                           loaded[loaded_count].size);
    }
    return failure_status;
}
