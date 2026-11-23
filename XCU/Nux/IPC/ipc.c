#include <nux/ipc.h>
extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);
typedef struct { nux_u32 id, head, tail, count; volatile nux_u32 lock; nux_ipc_message queue[NUX_IPC_QUEUE_DEPTH]; nux_u8 active; } nux_endpoint;
static nux_endpoint endpoints[NUX_IPC_ENDPOINT_LIMIT];
static volatile nux_u32 table_lock;
static nux_u32 next_id;
nux_status nux_ipc_system_init(void) {
 nux_arch_lock(&table_lock);
 for (nux_u32 i=0;i<NUX_IPC_ENDPOINT_LIMIT;i++){endpoints[i].id=0;endpoints[i].head=0;endpoints[i].tail=0;endpoints[i].count=0;endpoints[i].lock=0;endpoints[i].active=0;}
 next_id=1; nux_arch_unlock(&table_lock); return NUX_OK;
}
nux_status nux_ipc_endpoint_create(nux_endpoint_id *out) {
 if(!out)return NUX_E_INVALID; nux_arch_lock(&table_lock);
 for(nux_u32 i=0;i<NUX_IPC_ENDPOINT_LIMIT;i++) if(!endpoints[i].active){nux_endpoint *e=&endpoints[i];e->id=next_id++;if(!e->id)e->id=next_id++;e->head=e->tail=e->count=0;e->active=1;*out=e->id;nux_arch_unlock(&table_lock);return NUX_OK;}
 nux_arch_unlock(&table_lock);return NUX_E_LIMIT;
}
static nux_endpoint *lookup(nux_u32 id){for(nux_u32 i=0;i<NUX_IPC_ENDPOINT_LIMIT;i++)if(endpoints[i].active&&endpoints[i].id==id)return &endpoints[i];return NUX_NULL;}
nux_status nux_ipc_send(nux_capability cap,nux_u32 sender,const nux_ipc_message *m){
 nux_u32 id;if(!sender||!m||m->length>NUX_IPC_INLINE_BYTES)return NUX_E_INVALID;
 nux_status s=nux_capability_check(cap,NUX_OBJECT_ENDPOINT,NUX_RIGHT_WRITE,&id);if(s!=NUX_OK)return s;
 nux_arch_lock(&table_lock);nux_endpoint *e=lookup(id);nux_arch_unlock(&table_lock);if(!e)return NUX_E_NOT_FOUND;
 nux_arch_lock(&e->lock);if(e->count==NUX_IPC_QUEUE_DEPTH){nux_arch_unlock(&e->lock);return NUX_E_WOULD_BLOCK;}
 e->queue[e->tail]=*m;e->queue[e->tail].sender=sender;e->tail=(e->tail+1)%NUX_IPC_QUEUE_DEPTH;e->count++;nux_arch_unlock(&e->lock);return NUX_OK;
}
nux_status nux_ipc_receive(nux_capability cap,nux_u32 receiver,nux_ipc_message *m){
 nux_u32 id;if(!receiver||!m)return NUX_E_INVALID;
 nux_status s=nux_capability_check(cap,NUX_OBJECT_ENDPOINT,NUX_RIGHT_READ,&id);if(s!=NUX_OK)return s;
 nux_arch_lock(&table_lock);nux_endpoint *e=lookup(id);nux_arch_unlock(&table_lock);if(!e)return NUX_E_NOT_FOUND;
 nux_arch_lock(&e->lock);if(!e->count){nux_arch_unlock(&e->lock);return NUX_E_WOULD_BLOCK;}
 *m=e->queue[e->head];e->head=(e->head+1)%NUX_IPC_QUEUE_DEPTH;e->count--;nux_arch_unlock(&e->lock);return NUX_OK;
}
