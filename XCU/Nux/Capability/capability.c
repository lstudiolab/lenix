#include <nux/capability.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

typedef struct nux_capability_entry {
    nux_u32 generation;
    nux_u32 object_id;
    nux_u32 rights;
    nux_u8 type;
    nux_u8 active;
} nux_capability_entry;

static nux_capability_entry entries[NUX_CAPABILITY_LIMIT];
static volatile nux_u32 cap_lock;

nux_status nux_capability_system_init(void)
{
    nux_arch_lock(&cap_lock);
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        entries[i].generation = 1;
        entries[i].object_id = 0;
        entries[i].rights = 0;
        entries[i].type = NUX_OBJECT_NONE;
        entries[i].active = 0;
    }
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}

nux_status nux_capability_create(nux_object_type type, nux_u32 object_id,
                                 nux_u32 rights, nux_capability *out)
{
    if (out == NUX_NULL || type <= NUX_OBJECT_NONE ||
        type > NUX_OBJECT_MEMORY || object_id == 0 || rights == 0 ||
        (rights & ~(NUX_RIGHT_READ | NUX_RIGHT_WRITE |
                    NUX_RIGHT_GRANT | NUX_RIGHT_ADMIN)) != 0)
        return NUX_E_INVALID;

    nux_arch_lock(&cap_lock);
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        nux_capability_entry *entry = &entries[i];
        if (entry->active) continue;
        entry->active = 1;
        entry->type = (nux_u8)type;
        entry->object_id = object_id;
        entry->rights = rights;
        *out = (entry->generation << 8) | (i + 1);
        nux_arch_unlock(&cap_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&cap_lock);
    return NUX_E_LIMIT;
}

nux_status nux_capability_check(nux_capability cap, nux_object_type type,
                                nux_u32 required_rights, nux_u32 *object_id)
{
    nux_u32 slot = (cap & 0xffu);
    nux_u32 generation = cap >> 8;
    if (slot == 0 || slot > NUX_CAPABILITY_LIMIT || generation == 0 ||
        type <= NUX_OBJECT_NONE || object_id == NUX_NULL)
        return NUX_E_INVALID;

    nux_arch_lock(&cap_lock);
    nux_capability_entry *entry = &entries[slot - 1];
    if (!entry->active || entry->generation != generation ||
        entry->type != (nux_u8)type) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    if ((entry->rights & required_rights) != required_rights) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    *object_id = entry->object_id;
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}

nux_status nux_capability_revoke(nux_capability cap)
{
    nux_u32 slot = cap & 0xffu;
    nux_u32 generation = cap >> 8;
    if (slot == 0 || slot > NUX_CAPABILITY_LIMIT || generation == 0)
        return NUX_E_INVALID;

    nux_arch_lock(&cap_lock);
    nux_capability_entry *entry = &entries[slot - 1];
    if (!entry->active || entry->generation != generation) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_NOT_FOUND;
    }
    entry->active = 0;
    entry->object_id = 0;
    entry->rights = 0;
    entry->type = NUX_OBJECT_NONE;
    if (++entry->generation == 0 || entry->generation > 0x00ffffffu)
        entry->generation = 1;
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}
