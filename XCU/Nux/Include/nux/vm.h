#ifndef NUX_VM_H
#define NUX_VM_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/object.h>
#define NUX_VM_PROT_READ 1u
#define NUX_VM_PROT_WRITE 2u
#define NUX_VM_PROT_EXEC 4u
#define NUX_VM_INHERIT_NONE 0u
#define NUX_VM_INHERIT_COPY 1u
#define NUX_VM_INHERIT_SHARE 2u
typedef nux_u64 nux_vm_address;
typedef nux_u64 nux_vm_size;
typedef nux_u64 nux_memory_object_id;
typedef struct { nux_vm_address start; nux_vm_size size; nux_u32 protection; nux_u32 inheritance; nux_memory_object_id object; nux_u64 object_offset; } nux_vm_region;
nux_status nux_vm_system_init(void);
nux_status nux_vm_space_create(nux_u64 *space_id);
nux_status nux_vm_space_destroy(nux_u64 space_id);
nux_status nux_vm_map(nux_u64 space_id, nux_vm_address *address, nux_vm_size size, nux_u32 protection, nux_memory_object_id object, nux_u64 offset);
nux_status nux_vm_unmap(nux_u64 space_id, nux_vm_address address, nux_vm_size size);
nux_status nux_vm_protect(nux_u64 space_id, nux_vm_address address, nux_vm_size size, nux_u32 protection);
nux_status nux_vm_inherit(nux_u64 space_id, nux_vm_address address, nux_vm_size size, nux_u32 inheritance);
nux_status nux_vm_fault(nux_u64 space_id, nux_vm_address address, nux_u32 access);
nux_status nux_memory_object_create(nux_vm_size size, nux_memory_object_id *out);
nux_status nux_memory_object_map(nux_memory_object_id object, nux_u64 space_id, nux_vm_address *address, nux_vm_size size, nux_u32 protection);
nux_status nux_memory_object_lock(nux_memory_object_id object, nux_u64 offset, nux_vm_size size);
nux_status nux_memory_object_unlock(nux_memory_object_id object, nux_u64 offset, nux_vm_size size);
nux_status nux_memory_object_pager_register(nux_memory_object_id object, nux_capability pager);
nux_status nux_vm_copy_on_write(nux_u64 source_space, nux_u64 destination_space, nux_vm_address address, nux_vm_size size);
#endif
