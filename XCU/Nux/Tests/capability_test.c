#include <nux/capability.h>
#include <nux/task.h>

static nux_task_state task_states[3] = {
    NUX_TASK_UNUSED, NUX_TASK_READY, NUX_TASK_READY
};
void nux_arch_lock(volatile nux_u32 *lock){while(*lock){}*lock=1;}
void nux_arch_unlock(volatile nux_u32 *lock){*lock=0;}
nux_status nux_task_get(nux_task_id id,nux_task_info *out)
{
    if(!out||id<1||id>2)return NUX_E_NOT_FOUND;
    out->id=id;out->state=task_states[id];return NUX_OK;
}

int main(void)
{
    nux_capability first, derived, moved, one_shot, cap;
    nux_object_id object;
    if(nux_capability_system_init()!=NUX_OK)return 1;
    if(nux_capability_create(1,NUX_OBJECT_MEMORY,7,
       NUX_RIGHT_READ|NUX_RIGHT_TRANSFER,&first)!=NUX_OK)return 2;
    if(nux_capability_check(2,first,NUX_OBJECT_MEMORY,NUX_RIGHT_READ,&object)!=NUX_E_ACCESS)return 3;
    if(nux_capability_derive(1,first,NUX_RIGHT_TRANSFER,NUX_RIGHT_READ,&derived)!=NUX_OK)return 18;
    if(nux_capability_check(1,derived,NUX_OBJECT_MEMORY,NUX_RIGHT_READ,&object)!=NUX_OK)return 19;
    if(nux_capability_check(1,derived,NUX_OBJECT_MEMORY,NUX_RIGHT_TRANSFER,&object)!=NUX_E_ACCESS)return 20;
    if(nux_capability_move(1,first,2,NUX_OBJECT_MEMORY,
       NUX_RIGHT_TRANSFER|NUX_RIGHT_READ,NUX_RIGHT_READ,&object,&moved)!=NUX_OK)return 4;
    if(object!=7||nux_capability_check(1,first,NUX_OBJECT_MEMORY,NUX_RIGHT_READ,&object)!=NUX_E_ACCESS)return 5;
    if(nux_capability_check(2,moved,NUX_OBJECT_MEMORY,NUX_RIGHT_READ,&object)!=NUX_OK||object!=7)return 6;
    if(nux_capability_create(2,NUX_OBJECT_PORT,9,NUX_RIGHT_SEND_ONCE,&one_shot)!=NUX_OK)return 7;
    if(nux_capability_consume(2,one_shot,NUX_OBJECT_PORT,NUX_RIGHT_SEND_ONCE,&object)!=NUX_OK||object!=9)return 8;
    if(nux_capability_consume(2,one_shot,NUX_OBJECT_PORT,NUX_RIGHT_SEND_ONCE,&object)!=NUX_E_ACCESS)return 9;
    if(nux_capability_revoke_task(1)!=1)return 10;

    for(nux_u32 i=0;i<NUX_CAPABILITY_LIMIT;i++)
        if(nux_capability_create(1,NUX_OBJECT_MEMORY,i+1,NUX_RIGHT_READ,&cap)!=NUX_OK)return 11;
    if(nux_capability_create(1,NUX_OBJECT_MEMORY,999,NUX_RIGHT_READ,&cap)!=NUX_E_LIMIT)return 12;
    for(nux_u32 i=0;i<NUX_CAPABILITY_LIMIT-1;i++)
        if(nux_capability_create(2,NUX_OBJECT_MEMORY,i+1000,NUX_RIGHT_READ,&cap)!=NUX_OK)return 13;
    if(nux_capability_create(2,NUX_OBJECT_MEMORY,9999,NUX_RIGHT_READ,&cap)!=NUX_E_LIMIT)return 14;
    if(nux_capability_revoke_task(1)!=NUX_CAPABILITY_LIMIT)return 15;
    if(nux_capability_check(2,moved,NUX_OBJECT_MEMORY,NUX_RIGHT_READ,&object)!=NUX_OK)return 16;
    task_states[2]=NUX_TASK_DYING;
    if(nux_capability_create(2,NUX_OBJECT_MEMORY,1,NUX_RIGHT_READ,&cap)!=NUX_E_ACCESS)return 17;
    return 0;
}
