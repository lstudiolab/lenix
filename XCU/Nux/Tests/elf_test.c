#include <nux/elf.h>
#include <nux/task.h>
#include <nux/vm.h>

static nux_u8 image[0x3000];
static nux_u32 maps, writes, protects, unmaps;
static nux_u32 fail_stack_map;
nux_status nux_task_get_address_space(nux_task_id task,nux_u64 *space)
{if(task!=7||!space)return NUX_E_INVALID;*space=42;return NUX_OK;}
nux_status nux_vm_map(nux_u64 space,nux_vm_address *address,nux_vm_size size,nux_u32 prot,nux_memory_object_id object,nux_u64 offset)
{(void)size;(void)prot;(void)object;(void)offset;if(space!=42||!*address)return NUX_E_INVALID;if(fail_stack_map&&*address==NUX_ELF_STACK_TOP-NUX_ELF_STACK_SIZE)return NUX_E_NO_MEMORY;++maps;return NUX_OK;}
nux_status nux_vm_write(nux_u64 space,nux_vm_address address,const void *data,nux_size size)
{(void)address;(void)data;(void)size;if(space!=42)return NUX_E_INVALID;++writes;return NUX_OK;}
nux_status nux_vm_protect(nux_u64 space,nux_vm_address address,nux_vm_size size,nux_u32 prot)
{(void)address;(void)size;(void)prot;if(space!=42)return NUX_E_INVALID;++protects;return NUX_OK;}
nux_status nux_vm_unmap(nux_u64 space,nux_vm_address address,nux_vm_size size)
{(void)address;(void)size;if(space!=42)return NUX_E_INVALID;++unmaps;return NUX_OK;}

static void put16(nux_u8 *p,nux_u16 v){p[0]=(nux_u8)v;p[1]=(nux_u8)(v>>8);}
static void put32(nux_u8 *p,nux_u32 v){for(nux_u32 i=0;i<4;i++)p[i]=(nux_u8)(v>>(i*8));}
static void put64(nux_u8 *p,nux_u64 v){for(nux_u32 i=0;i<8;i++)p[i]=(nux_u8)(v>>(i*8));}
static void make_image(void)
{
    image[0]=0x7f;image[1]='E';image[2]='L';image[3]='F';
    image[4]=2;image[5]=1;image[6]=1;
    put16(image+16,2);
#if defined(NUX_ELF_TEST_X86_64)
    put16(image+18,62);
#else
    put16(image+18,183);
#endif
    put32(image+20,1);put64(image+24,0x10000080);
    put64(image+32,64);put16(image+52,64);put16(image+54,56);put16(image+56,2);
    nux_u8 *p=image+64;
    put32(p,1);put32(p+4,5);put64(p+8,0x1000);put64(p+16,0x10000000);
    put64(p+32,16);put64(p+40,4096);put64(p+48,4096);
    p+=56;put32(p,1);put32(p+4,6);put64(p+8,0x2000);put64(p+16,0x10001000);
    put64(p+32,8);put64(p+40,4096);put64(p+48,4096);
    for(nux_u32 i=0;i<16;i++)image[0x1000+i]=(nux_u8)(i+1);
    for(nux_u32 i=0;i<8;i++)image[0x2000+i]=(nux_u8)(0xa0+i);
}
int main(void)
{
    nux_elf_load_result result;
    make_image();
    if(nux_elf_load(7,image,sizeof(image),&result)!=NUX_OK)return 1;
    if(result.entry!=0x10000080||result.segment_count!=2||maps!=3||writes!=3||protects!=2)return 2;
    if(result.stack_base!=0x00007fff00000000ULL-65536ULL||
       result.stack_pointer!=NUX_ELF_STACK_TOP-80ULL||
       result.stack_size!=65536ULL||(result.stack_pointer&15ULL))return 8;
    fail_stack_map=1;
    if(nux_elf_load(7,image,sizeof(image),&result)!=NUX_E_NO_MEMORY)return 9;
    if(result.entry||result.stack_pointer||unmaps!=2)return 10;
    fail_stack_map=0;
    if(nux_elf_load(7,image,100,&result)!=NUX_E_BAD_STATE)return 3;
    put32(image+64+56,2);
    if(nux_elf_load(7,image,sizeof(image),&result)!=NUX_E_NOT_SUPPORTED)return 4;
    if(unmaps!=3)return 5;
    put32(image+64+56,1);
    put64(image+64+56+40,4);
    if(nux_elf_load(7,image,sizeof(image),&result)!=NUX_E_BAD_STATE)return 6;
    if(unmaps!=4)return 7;
    return 0;
}
