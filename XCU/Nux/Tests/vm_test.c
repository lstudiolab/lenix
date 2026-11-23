#include <nux/vm.h>
#include <nux/memory.h>
#include <nux/task.h>

#define PAGE_COUNT 128u
#define MAP_LIMIT 512u
static nux_u8 arena[PAGE_COUNT*NUX_PAGE_SIZE];
static nux_u8 allocated[PAGE_COUNT];
static nux_u64 next_root=1;
typedef struct {nux_u64 root,va,pa;nux_u32 prot;nux_u8 active;} map_entry;
static map_entry maps[MAP_LIMIT];

void nux_arch_lock(volatile nux_u32 *lock){while(*lock){}*lock=1;}
void nux_arch_unlock(volatile nux_u32 *lock){*lock=0;}
nux_u64 nux_phys_alloc_page(void){
    for(nux_u32 i=1;i<PAGE_COUNT;i++)if(!allocated[i]){allocated[i]=1;return (nux_u64)i*NUX_PAGE_SIZE;}
    return 0;
}
nux_status nux_phys_free_page(nux_u64 pa){
    if(!pa||(pa&NUX_PAGE_MASK)||pa/NUX_PAGE_SIZE>=PAGE_COUNT||!allocated[pa/NUX_PAGE_SIZE])return NUX_E_ACCESS;
    allocated[pa/NUX_PAGE_SIZE]=0;return NUX_OK;
}
void *nux_phys_to_virt(nux_u64 pa){if(pa>=sizeof(arena))return NUX_NULL;return arena+pa;}
nux_status nux_arch_vm_root_create(nux_u64 *out){if(!out)return NUX_E_INVALID;*out=next_root++;return NUX_OK;}
nux_status nux_arch_vm_map(nux_u64 root,nux_u64 va,nux_u64 pa,nux_u32 prot){
    for(nux_u32 i=0;i<MAP_LIMIT;i++)if(maps[i].active&&maps[i].root==root&&maps[i].va==va)return NUX_E_BUSY;
    for(nux_u32 i=0;i<MAP_LIMIT;i++)if(!maps[i].active){maps[i]=(map_entry){root,va,pa,prot,1};return NUX_OK;}
    return NUX_E_LIMIT;
}
nux_status nux_arch_vm_unmap(nux_u64 root,nux_u64 va){
    for(nux_u32 i=0;i<MAP_LIMIT;i++)if(maps[i].active&&maps[i].root==root&&maps[i].va==va){maps[i].active=0;return NUX_OK;}
    return NUX_E_NOT_FOUND;
}
nux_status nux_arch_vm_protect(nux_u64 root,nux_u64 va,nux_u32 prot){
    for(nux_u32 i=0;i<MAP_LIMIT;i++)if(maps[i].active&&maps[i].root==root&&maps[i].va==va){maps[i].prot=prot;return NUX_OK;}
    return NUX_E_NOT_FOUND;
}
void nux_arch_vm_root_destroy(nux_u64 root){for(nux_u32 i=0;i<MAP_LIMIT;i++)if(maps[i].active&&maps[i].root==root)maps[i].active=0;}
nux_status nux_arch_vm_activate(nux_u64 root){(void)root;return NUX_OK;}
nux_u64 nux_arch_vm_translate(nux_u64 root,nux_u64 va){
    for(nux_u32 i=0;i<MAP_LIMIT;i++)if(maps[i].active&&maps[i].root==root&&maps[i].va==(va&~NUX_PAGE_MASK))return maps[i].pa+(va&NUX_PAGE_MASK);
    return 0;
}
nux_status nux_capability_check(nux_capability cap,nux_object_type type,nux_rights rights,nux_object_id *out){
    if(!cap||type!=NUX_OBJECT_PORT||rights!=NUX_RIGHT_SEND||!out)
        return NUX_E_ACCESS;
    *out=1;
    return NUX_OK;
}
int main(void){
    nux_task_id task_a=0,task_b=0;
    nux_u64 task_space_a=0,task_space_b=0;
    nux_u64 space=0; nux_vm_address address=0; nux_memory_object_id object=0;
    if(nux_vm_system_init()!=NUX_OK)return 1;
    if(nux_task_system_init()!=NUX_OK)return 19;
    if(nux_task_create("vm-task-a",10,&task_a)!=NUX_OK)return 20;
    if(nux_task_create("vm-task-b",10,&task_b)!=NUX_OK)return 21;
    if(nux_task_get_address_space(task_a,&task_space_a)!=NUX_OK)return 22;
    if(nux_task_get_address_space(task_b,&task_space_b)!=NUX_OK)return 23;
    if(!task_space_a||!task_space_b||task_space_a==task_space_b)return 24;
    if(nux_vm_space_destroy(task_space_a)!=NUX_OK)return 25;
    if(nux_vm_space_destroy(task_space_b)!=NUX_OK)return 26;
    if(nux_vm_space_create(&space)!=NUX_OK)return 2;
    if(nux_vm_map(space,&address,2*NUX_VM_PAGE_SIZE,NUX_VM_PROT_READ|NUX_VM_PROT_WRITE,0,0)!=NUX_OK)return 3;
    if(address<0x10000000ULL||nux_vm_fault(space,address,NUX_VM_PROT_READ)!=NUX_OK)return 4;
    if(nux_vm_fault(space,address,NUX_VM_PROT_EXEC)!=NUX_E_ACCESS)return 5;
    if(nux_vm_protect(space,address,2*NUX_VM_PAGE_SIZE,NUX_VM_PROT_READ)!=NUX_OK)return 6;
    if(nux_vm_fault(space,address,NUX_VM_PROT_WRITE)!=NUX_E_ACCESS)return 7;
    if(nux_vm_unmap(space,address,2*NUX_VM_PAGE_SIZE)!=NUX_OK)return 8;
    if(nux_memory_object_create(2*NUX_VM_PAGE_SIZE,&object)!=NUX_OK)return 9;
    address=0;
    if(nux_memory_object_map(object,space,&address,2*NUX_VM_PAGE_SIZE,NUX_VM_PROT_READ|NUX_VM_PROT_WRITE)!=NUX_OK)return 10;
    if(nux_memory_object_destroy(object)!=NUX_E_BUSY)return 11;
    if(nux_memory_object_lock(object,0,NUX_VM_PAGE_SIZE)!=NUX_OK)return 12;
    if(nux_memory_object_unlock(object,0,NUX_VM_PAGE_SIZE)!=NUX_OK)return 13;
    if(nux_vm_unmap(space,address,2*NUX_VM_PAGE_SIZE)!=NUX_OK)return 14;
    if(nux_memory_object_destroy(object)!=NUX_OK)return 15;
    if(nux_vm_space_activate(space)!=NUX_OK)return 16;
    if(nux_vm_space_activate(0)!=NUX_OK)return 27;
    if(nux_vm_space_destroy(space)!=NUX_OK)return 17;
    if(nux_vm_space_destroy(space)!=NUX_E_NOT_FOUND)return 18;
    return 0;
}
