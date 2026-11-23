#include <nux/vm.h>
#include <nux/memory.h>

#define NUX_VM_SPACE_LIMIT 64u
#define NUX_VM_REGION_LIMIT 128u
#define NUX_VM_OBJECT_LIMIT 128u
#define NUX_VM_USER_BASE 0x0000000010000000ULL
#define NUX_VM_USER_END  0x00007fff00000000ULL
#define NUX_VM_VALID_PROT (NUX_VM_PROT_READ|NUX_VM_PROT_WRITE|NUX_VM_PROT_EXEC)

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);
extern void *memset(void *dst, int value, nux_size size);
extern nux_status nux_arch_vm_root_create(nux_u64 *root);
extern nux_status nux_arch_vm_map(nux_u64 root, nux_u64 va, nux_u64 pa, nux_u32 prot);
extern nux_status nux_arch_vm_unmap(nux_u64 root, nux_u64 va);
extern nux_status nux_arch_vm_protect(nux_u64 root, nux_u64 va, nux_u32 prot);
extern void nux_arch_vm_root_destroy(nux_u64 root);
extern nux_status nux_arch_vm_activate(nux_u64 root);
extern nux_u64 nux_arch_vm_translate(nux_u64 root, nux_u64 va);

typedef struct {
    nux_u64 id, root;
    nux_vm_region regions[NUX_VM_REGION_LIMIT];
    nux_u32 count;
    nux_u8 active;
} nux_vm_space;
typedef struct {
    nux_u64 id, size;
    nux_u64 pages[NUX_VM_OBJECT_MAX_PAGES];
    nux_u32 page_count, locks;
    nux_capability pager;
    nux_u8 active;
} nux_vm_object;

static nux_vm_space spaces[NUX_VM_SPACE_LIMIT];
static nux_vm_object objects[NUX_VM_OBJECT_LIMIT];
static volatile nux_u32 vm_lock;
static nux_u64 next_space_id, next_object_id, active_space_id;

static nux_vm_space *space_find(nux_u64 id) {
    for (nux_u32 i=0;i<NUX_VM_SPACE_LIMIT;i++) if(spaces[i].active&&spaces[i].id==id)return &spaces[i];
    return NUX_NULL;
}
static nux_vm_object *object_find(nux_u64 id) {
    for (nux_u32 i=0;i<NUX_VM_OBJECT_LIMIT;i++) if(objects[i].active&&objects[i].id==id)return &objects[i];
    return NUX_NULL;
}
static nux_u64 align_up(nux_u64 v) { return (v+NUX_VM_PAGE_SIZE-1)&~(NUX_VM_PAGE_SIZE-1); }
static nux_status protection_valid(nux_u32 p) {
    if(!(p&NUX_VM_PROT_READ)||(p&~NUX_VM_VALID_PROT)||((p&NUX_VM_PROT_WRITE)&&(p&NUX_VM_PROT_EXEC)))return NUX_E_INVALID;
    return NUX_OK;
}
static nux_status region_insert(nux_vm_space *s,nux_vm_region r) {
    if(s->count==NUX_VM_REGION_LIMIT)return NUX_E_LIMIT;
    for(nux_u32 i=0;i<s->count;i++) {
        nux_u64 a=s->regions[i].start,b=a+s->regions[i].size;
        if(r.start < b && a < r.start+r.size)return NUX_E_BUSY;
    }
    nux_u32 i=s->count++;
    while(i&&s->regions[i-1].start>r.start){s->regions[i]=s->regions[i-1];--i;}
    s->regions[i]=r;return NUX_OK;
}
static nux_status region_remove(nux_vm_space *s,nux_u32 index) {
    if(index>=s->count)return NUX_E_NOT_FOUND;
    for(nux_u32 i=index+1;i<s->count;i++)s->regions[i-1]=s->regions[i];
    --s->count;return NUX_OK;
}
static nux_status choose_address(nux_vm_space *s,nux_u64 size,nux_u64 *out) {
    nux_u64 at=NUX_VM_USER_BASE;
    for(nux_u32 i=0;i<s->count;i++) {
        if(at<=s->regions[i].start && size<=s->regions[i].start-at){*out=at;return NUX_OK;}
        if(s->regions[i].start+s->regions[i].size>at)at=align_up(s->regions[i].start+s->regions[i].size);
    }
    if(at>=NUX_VM_USER_END||size>NUX_VM_USER_END-at)return NUX_E_NO_MEMORY;
    *out=at;return NUX_OK;
}
nux_status nux_vm_system_init(void) {
    nux_arch_lock(&vm_lock);
    for(nux_u32 i=0;i<NUX_VM_SPACE_LIMIT;i++)spaces[i].active=0;
    for(nux_u32 i=0;i<NUX_VM_OBJECT_LIMIT;i++)objects[i].active=0;
    next_space_id=1;next_object_id=1;active_space_id=0;
    nux_arch_unlock(&vm_lock);return NUX_OK;
}
nux_status nux_vm_space_create(nux_u64 *out) {
    if(!out)return NUX_E_INVALID;
    nux_u64 root; nux_status st=nux_arch_vm_root_create(&root);if(st!=NUX_OK)return st;
    nux_arch_lock(&vm_lock);
    for(nux_u32 i=0;i<NUX_VM_SPACE_LIMIT;i++)if(!spaces[i].active){
        spaces[i].id=next_space_id++;if(!spaces[i].id)spaces[i].id=next_space_id++;
        spaces[i].root=root;spaces[i].count=0;spaces[i].active=1;*out=spaces[i].id;
        nux_arch_unlock(&vm_lock);return NUX_OK;
    }
    nux_arch_unlock(&vm_lock);nux_arch_vm_root_destroy(root);return NUX_E_LIMIT;
}
nux_status nux_vm_space_destroy(nux_u64 id) {
    nux_arch_lock(&vm_lock);nux_vm_space *s=space_find(id);
    if(!s){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    nux_u64 root=s->root;
    for(nux_u32 i=0;i<s->count;i++){nux_vm_region *r=&s->regions[i];if(!r->object)for(nux_u64 off=0;off<r->size;off+=NUX_VM_PAGE_SIZE){nux_u64 pa=nux_arch_vm_translate(root,r->start+off);if(pa)(void)nux_phys_free_page(pa&~(NUX_VM_PAGE_SIZE-1));}}
    s->active=0;s->count=0;if(active_space_id==id){active_space_id=0;(void)nux_arch_vm_activate(0);}nux_arch_unlock(&vm_lock);
    nux_arch_vm_root_destroy(root);return NUX_OK;
}
nux_status nux_vm_space_activate(nux_u64 id) {
    nux_arch_lock(&vm_lock);
    if (!id) {
        nux_status restore = nux_arch_vm_activate(0);
        if (restore == NUX_OK) active_space_id = 0;
        nux_arch_unlock(&vm_lock);
        return restore;
    }
    nux_vm_space *s=space_find(id);
    if(!s){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    nux_status st=nux_arch_vm_activate(s->root);
    if(st==NUX_OK)active_space_id=id;
    nux_arch_unlock(&vm_lock);
    return st;
}
nux_status nux_vm_map(nux_u64 id,nux_vm_address *address,nux_vm_size size,nux_u32 prot,nux_memory_object_id object,nux_u64 offset) {
    if(!address||!size||protection_valid(prot)!=NUX_OK||(size&(NUX_VM_PAGE_SIZE-1))||(offset&(NUX_VM_PAGE_SIZE-1)))return NUX_E_INVALID;
    nux_arch_lock(&vm_lock);nux_vm_space *s=space_find(id);nux_vm_object *o=object?object_find(object):NUX_NULL;
    if(!s||(object&&!o)){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    if(o&&(offset>o->size||size>o->size-offset)){nux_arch_unlock(&vm_lock);return NUX_E_INVALID;}
    nux_u64 va=*address;
    if(!va){nux_status st=choose_address(s,size,&va);if(st!=NUX_OK){nux_arch_unlock(&vm_lock);return st;}}
    if((va&(NUX_VM_PAGE_SIZE-1))||va<NUX_VM_USER_BASE||va>=NUX_VM_USER_END||size>NUX_VM_USER_END-va){nux_arch_unlock(&vm_lock);return NUX_E_INVALID;}
    nux_vm_region r={va,size,prot,NUX_VM_INHERIT_NONE,object,offset};
    nux_status st=region_insert(s,r);if(st!=NUX_OK){nux_arch_unlock(&vm_lock);return st;}
    nux_u64 root=s->root;
    nux_u32 first_page=o?(nux_u32)(offset/NUX_VM_PAGE_SIZE):0;
    nux_u32 pages=(nux_u32)(size/NUX_VM_PAGE_SIZE);
    nux_u32 mapped=0;
    for(;mapped<pages;mapped++){
        nux_u64 pa;
        if(o)pa=o->pages[first_page+mapped];
        else {pa=nux_phys_alloc_page();if(!pa){st=NUX_E_NO_MEMORY;break;}memset(nux_phys_to_virt(pa),0,NUX_VM_PAGE_SIZE);}
        st=nux_arch_vm_map(root,va+(nux_u64)mapped*NUX_VM_PAGE_SIZE,pa,prot);
        if(st!=NUX_OK){if(!o)(void)nux_phys_free_page(pa);break;}
    }
    if(mapped!=pages){
        while(mapped){--mapped;nux_u64 pa=nux_arch_vm_translate(root,va+(nux_u64)mapped*NUX_VM_PAGE_SIZE);(void)nux_arch_vm_unmap(root,va+(nux_u64)mapped*NUX_VM_PAGE_SIZE);if(!o)(void)nux_phys_free_page(pa&~(NUX_VM_PAGE_SIZE-1));}
        for(nux_u32 i=0;i<s->count;i++) {
            if(s->regions[i].start==va) { (void)region_remove(s,i); break; }
        }
        nux_arch_unlock(&vm_lock);
        return st;
    }
    *address=va;nux_arch_unlock(&vm_lock);return NUX_OK;
}
nux_status nux_vm_unmap(nux_u64 id,nux_vm_address va,nux_vm_size size) {
    if(!size||(va&(NUX_VM_PAGE_SIZE-1))||(size&(NUX_VM_PAGE_SIZE-1)))return NUX_E_INVALID;
    nux_arch_lock(&vm_lock);nux_vm_space *s=space_find(id);if(!s){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    nux_u32 index=s->count;
    for(nux_u32 i=0;i<s->count;i++)if(s->regions[i].start==va&&s->regions[i].size==size){index=i;break;}
    if(index==s->count){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    nux_vm_region r=s->regions[index];
    for(nux_u64 off=0;off<size;off+=NUX_VM_PAGE_SIZE){
        nux_u64 pa=nux_arch_vm_translate(s->root,va+off);
        (void)nux_arch_vm_unmap(s->root,va+off);
        if(!r.object&&pa)(void)nux_phys_free_page(pa&~(NUX_VM_PAGE_SIZE-1));
    }
    (void)region_remove(s,index);nux_arch_unlock(&vm_lock);return NUX_OK;
}

nux_status nux_vm_write(nux_u64 id, nux_vm_address address,
                        const void *source, nux_size size)
{
    const nux_u8 *input = (const nux_u8 *)source;
    if (!source || !size || address > NUX_U64_MAX - size)
        return NUX_E_INVALID;
    nux_arch_lock(&vm_lock);
    nux_vm_space *space = space_find(id);
    if (!space) {
        nux_arch_unlock(&vm_lock);
        return NUX_E_NOT_FOUND;
    }
    nux_vm_region *region = NUX_NULL;
    for (nux_u32 i = 0; i < space->count; ++i) {
        nux_vm_region *candidate = &space->regions[i];
        if (address >= candidate->start &&
            address - candidate->start <= candidate->size &&
            size <= candidate->size - (address - candidate->start)) {
            region = candidate;
            break;
        }
    }
    if (!region || !(region->protection & NUX_VM_PROT_WRITE)) {
        nux_arch_unlock(&vm_lock);
        return NUX_E_ACCESS;
    }
    nux_u64 end = address + size;
    for (nux_u64 cursor = address; cursor < end;) {
        nux_u64 page = cursor & ~(NUX_VM_PAGE_SIZE - 1);
        if (!nux_arch_vm_translate(space->root, page)) {
            nux_arch_unlock(&vm_lock);
            return NUX_E_NOT_FOUND;
        }
        nux_u64 amount = NUX_VM_PAGE_SIZE -
            (cursor & (NUX_VM_PAGE_SIZE - 1));
        if (amount > end - cursor) amount = end - cursor;
        cursor += amount;
    }
    nux_u64 cursor = address;
    while (cursor < end) {
        nux_u64 page = cursor & ~(NUX_VM_PAGE_SIZE - 1);
        nux_u64 physical = nux_arch_vm_translate(space->root, page);
        nux_u64 offset = cursor & (NUX_VM_PAGE_SIZE - 1);
        nux_u64 amount = NUX_VM_PAGE_SIZE - offset;
        nux_u64 region_remaining = region->start + region->size - cursor;
        if (amount > end - cursor) amount = end - cursor;
        if (amount > region_remaining) amount = region_remaining;
        nux_u8 *output = (nux_u8 *)nux_phys_to_virt(physical) + offset;
        for (nux_u64 i = 0; i < amount; ++i)
            output[i] = input[i];
        input += amount;
        cursor += amount;
    }
    nux_arch_unlock(&vm_lock);
    return NUX_OK;
}


nux_status nux_vm_read(nux_u64 id, nux_vm_address address,
                       void *destination, nux_size size)
{
    nux_u8 *output = (nux_u8 *)destination;
    if (!destination || !size || address > NUX_U64_MAX - size)
        return NUX_E_INVALID;
    nux_arch_lock(&vm_lock);
    nux_vm_space *space = space_find(id);
    if (!space) {
        nux_arch_unlock(&vm_lock);
        return NUX_E_NOT_FOUND;
    }
    nux_u64 end = address + size;
    nux_u64 cursor = address;
    while (cursor < end) {
        nux_vm_region *region = NUX_NULL;
        for (nux_u32 i = 0; i < space->count; ++i) {
            nux_vm_region *candidate = &space->regions[i];
            if (cursor >= candidate->start &&
                cursor - candidate->start < candidate->size) {
                region = candidate;
                break;
            }
        }
        if (!region || !(region->protection & NUX_VM_PROT_READ)) {
            nux_arch_unlock(&vm_lock);
            return NUX_E_ACCESS;
        }
        nux_u64 page = cursor & ~(NUX_VM_PAGE_SIZE - 1);
        nux_u64 physical = nux_arch_vm_translate(space->root, page);
        if (!physical) {
            nux_arch_unlock(&vm_lock);
            return NUX_E_NOT_FOUND;
        }
        nux_u64 offset = cursor & (NUX_VM_PAGE_SIZE - 1);
        nux_u64 amount = NUX_VM_PAGE_SIZE - offset;
        nux_u64 region_remaining = region->start + region->size - cursor;
        if (amount > end - cursor) amount = end - cursor;
        if (amount > region_remaining) amount = region_remaining;
        const nux_u8 *input = (const nux_u8 *)nux_phys_to_virt(physical) + offset;
        for (nux_u64 i = 0; i < amount; ++i)
            output[i] = input[i];
        output += amount;
        cursor += amount;
    }
    nux_arch_unlock(&vm_lock);
    return NUX_OK;
}
\nnux_status nux_vm_protect(nux_u64 id,nux_vm_address va,nux_vm_size size,nux_u32 prot) {
    if(!size||protection_valid(prot)!=NUX_OK||(va&(NUX_VM_PAGE_SIZE-1))||(size&(NUX_VM_PAGE_SIZE-1)))return NUX_E_INVALID;
    nux_arch_lock(&vm_lock);nux_vm_space *s=space_find(id);if(!s){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    for(nux_u32 i=0;i<s->count;i++)if(s->regions[i].start==va&&s->regions[i].size==size){
        for(nux_u64 off=0;off<size;off+=NUX_VM_PAGE_SIZE){nux_status st=nux_arch_vm_protect(s->root,va+off,prot);if(st!=NUX_OK){nux_arch_unlock(&vm_lock);return st;}}
        s->regions[i].protection=prot;nux_arch_unlock(&vm_lock);return NUX_OK;
    }
    nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;
}
nux_status nux_vm_inherit(nux_u64 id,nux_vm_address va,nux_vm_size size,nux_u32 inheritance) {
    if(inheritance>NUX_VM_INHERIT_SHARE||!size)return NUX_E_INVALID;
    nux_arch_lock(&vm_lock);nux_vm_space *s=space_find(id);if(!s){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    for(nux_u32 i=0;i<s->count;i++)if(s->regions[i].start==va&&s->regions[i].size==size){s->regions[i].inheritance=inheritance;nux_arch_unlock(&vm_lock);return NUX_OK;}
    nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;
}
nux_status nux_vm_fault(nux_u64 id,nux_vm_address va,nux_u32 access) {
    if(!access||(access&~NUX_VM_VALID_PROT)||((access&NUX_VM_PROT_WRITE)&&(access&NUX_VM_PROT_EXEC)))return NUX_E_INVALID;
    nux_arch_lock(&vm_lock);nux_vm_space *s=space_find(id);if(!s){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    for(nux_u32 i=0;i<s->count;i++){nux_vm_region *r=&s->regions[i];if(va>=r->start&&va-r->start<r->size){nux_status st=((r->protection&access)==access)?NUX_OK:NUX_E_ACCESS;nux_arch_unlock(&vm_lock);return st;}}
    nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;
}
nux_status nux_memory_object_create(nux_vm_size size,nux_memory_object_id *out) {
    if(!out||!size||(size&(NUX_VM_PAGE_SIZE-1))||size/NUX_VM_PAGE_SIZE>NUX_VM_OBJECT_MAX_PAGES)return NUX_E_INVALID;
    nux_vm_object *o=NUX_NULL;nux_arch_lock(&vm_lock);
    for(nux_u32 i=0;i<NUX_VM_OBJECT_LIMIT;i++)if(!objects[i].active){o=&objects[i];break;}
    if(!o){nux_arch_unlock(&vm_lock);return NUX_E_LIMIT;}
    o->id=next_object_id++;if(!o->id)o->id=next_object_id++;o->size=size;o->page_count=(nux_u32)(size/NUX_VM_PAGE_SIZE);o->locks=0;o->pager=0;
    nux_u32 made=0;
    for(;made<o->page_count;made++){o->pages[made]=nux_phys_alloc_page();if(!o->pages[made])break;memset(nux_phys_to_virt(o->pages[made]),0,NUX_VM_PAGE_SIZE);}
    if(made!=o->page_count){while(made)(void)nux_phys_free_page(o->pages[--made]);o->page_count=0;nux_arch_unlock(&vm_lock);return NUX_E_NO_MEMORY;}
    o->active=1;*out=o->id;nux_arch_unlock(&vm_lock);return NUX_OK;
}
nux_status nux_memory_object_destroy(nux_memory_object_id id) {
    nux_arch_lock(&vm_lock);nux_vm_object *o=object_find(id);if(!o){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    if(o->locks){nux_arch_unlock(&vm_lock);return NUX_E_BUSY;}
    for(nux_u32 s=0;s<NUX_VM_SPACE_LIMIT;s++)if(spaces[s].active)for(nux_u32 r=0;r<spaces[s].count;r++)if(spaces[s].regions[r].object==id){nux_arch_unlock(&vm_lock);return NUX_E_BUSY;}
    o->active=0;nux_u32 count=o->page_count;nux_arch_unlock(&vm_lock);
    for(nux_u32 i=0;i<count;i++)(void)nux_phys_free_page(o->pages[i]);
    return NUX_OK;
}
nux_status nux_memory_object_map(nux_memory_object_id object,nux_u64 space,nux_vm_address *address,nux_vm_size size,nux_u32 prot) {
    return nux_vm_map(space,address,size,prot,object,0);
}
nux_status nux_memory_object_lock(nux_memory_object_id id,nux_u64 offset,nux_vm_size size) {
    nux_arch_lock(&vm_lock);nux_vm_object *o=object_find(id);if(!o){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    if(!size||offset>o->size||size>o->size-offset){nux_arch_unlock(&vm_lock);return NUX_E_INVALID;}++o->locks;nux_arch_unlock(&vm_lock);return NUX_OK;
}
nux_status nux_memory_object_unlock(nux_memory_object_id id,nux_u64 offset,nux_vm_size size) {
    nux_arch_lock(&vm_lock);nux_vm_object *o=object_find(id);if(!o){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}
    if(!size||offset>o->size||size>o->size-offset||!o->locks){nux_arch_unlock(&vm_lock);return NUX_E_INVALID;}--o->locks;nux_arch_unlock(&vm_lock);return NUX_OK;
}
nux_status nux_memory_object_pager_register(nux_task_id owner,nux_memory_object_id id,nux_capability pager) {
    nux_object_id object; nux_status st=nux_capability_check(owner,pager,NUX_OBJECT_PORT,NUX_RIGHT_SEND,&object);if(st!=NUX_OK)return st;
    nux_arch_lock(&vm_lock);nux_vm_object *o=object_find(id);if(!o){nux_arch_unlock(&vm_lock);return NUX_E_NOT_FOUND;}o->pager=pager;nux_arch_unlock(&vm_lock);return NUX_OK;
}
nux_status nux_vm_copy_on_write(nux_u64 source,nux_u64 destination,nux_vm_address va,nux_vm_size size) {
    (void)source;(void)destination;(void)va;(void)size;return NUX_E_NOT_SUPPORTED;
}
