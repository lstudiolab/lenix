#include <nux/capability.h>
extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);
typedef struct { nux_u32 generation; nux_u32 object_id; nux_rights rights; nux_object_type type; nux_u8 active; } nux_capability_entry;
static nux_capability_entry entries[NUX_CAPABILITY_LIMIT];
static volatile nux_u32 cap_lock;
nux_status nux_capability_system_init(void) {
 nux_arch_lock(&cap_lock);
 for(nux_u32 i=0;i<NUX_CAPABILITY_LIMIT;i++){entries[i].generation=1;entries[i].object_id=0;entries[i].rights=0;entries[i].type=NUX_OBJECT_NONE;entries[i].active=0;}
 nux_arch_unlock(&cap_lock);return NUX_OK;
}
nux_status nux_capability_create(nux_object_type type,nux_u32 object_id,nux_rights rights,nux_capability *out) {
 const nux_rights allowed=NUX_RIGHT_READ|NUX_RIGHT_WRITE|NUX_RIGHT_SEND|NUX_RIGHT_RECEIVE|NUX_RIGHT_MAP|NUX_RIGHT_CONTROL|NUX_RIGHT_TRANSFER;
 if(!out||type<=NUX_OBJECT_NONE||type>NUX_OBJECT_MEMORY||!object_id||!rights||(rights&~allowed))return NUX_E_INVALID;
 nux_arch_lock(&cap_lock);
 for(nux_u32 i=0;i<NUX_CAPABILITY_LIMIT;i++){nux_capability_entry *e=&entries[i];if(e->active)continue;e->active=1;e->type=type;e->object_id=object_id;e->rights=rights;*out=(e->generation<<8)|(i+1);nux_arch_unlock(&cap_lock);return NUX_OK;}
 nux_arch_unlock(&cap_lock);return NUX_E_LIMIT;
}
nux_status nux_capability_check(nux_capability cap,nux_object_type type,nux_rights required,nux_u32 *object_id) {
 nux_u32 slot=cap&0xffu,generation=cap>>8;if(!slot||slot>NUX_CAPABILITY_LIMIT||!generation||!object_id||type==NUX_OBJECT_NONE)return NUX_E_INVALID;
 nux_arch_lock(&cap_lock);nux_capability_entry *e=&entries[slot-1];
 if(!e->active||e->generation!=generation||e->type!=type){nux_arch_unlock(&cap_lock);return NUX_E_ACCESS;}
 if((e->rights&required)!=required){nux_arch_unlock(&cap_lock);return NUX_E_ACCESS;}
 *object_id=e->object_id;nux_arch_unlock(&cap_lock);return NUX_OK;
}
nux_status nux_capability_revoke(nux_capability cap) {
 nux_u32 slot=cap&0xffu,generation=cap>>8;if(!slot||slot>NUX_CAPABILITY_LIMIT||!generation)return NUX_E_INVALID;
 nux_arch_lock(&cap_lock);nux_capability_entry *e=&entries[slot-1];
 if(!e->active||e->generation!=generation){nux_arch_unlock(&cap_lock);return NUX_E_NOT_FOUND;}
 e->active=0;e->object_id=0;e->rights=0;e->type=NUX_OBJECT_NONE;if(++e->generation==0||e->generation>0x00ffffffu)e->generation=1;
 nux_arch_unlock(&cap_lock);return NUX_OK;
}
