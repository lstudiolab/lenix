#include <nux/capability.h>
#include <nux/task.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

typedef struct {
    nux_u32 generation;
    nux_task_id owner;
    nux_object_id object_id;
    nux_rights rights;
    nux_object_type type;
    nux_u8 active;
} nux_capability_entry;

static nux_capability_entry entries[NUX_CAPABILITY_LIMIT];
static volatile nux_u32 cap_lock;

static void nux_capability_entry_revoke(nux_capability_entry *entry)
{
    entry->active = 0;
    entry->owner = 0;
    entry->object_id = 0;
    entry->rights = 0;
    entry->type = NUX_OBJECT_NONE;
    if (++entry->generation == 0 || entry->generation > 0x00ffffffu)
        entry->generation = 1;
}

nux_status nux_capability_system_init(void)
{
    nux_arch_lock(&cap_lock);
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        entries[i].generation = 1;
        entries[i].owner = 0;
        entries[i].object_id = 0;
        entries[i].rights = 0;
        entries[i].type = NUX_OBJECT_NONE;
        entries[i].active = 0;
    }
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}

nux_status nux_capability_create(nux_task_id owner, nux_object_type type,
                                 nux_object_id object_id, nux_rights rights,
                                 nux_capability *out)
{
    const nux_rights allowed = NUX_RIGHT_READ | NUX_RIGHT_WRITE |
        NUX_RIGHT_SEND | NUX_RIGHT_RECEIVE | NUX_RIGHT_MAP |
        NUX_RIGHT_CONTROL | NUX_RIGHT_TRANSFER | NUX_RIGHT_SEND_ONCE;
    nux_task_info task;
    if (!owner || !out || type <= NUX_OBJECT_NONE ||
        type > NUX_OBJECT_MEMORY || !object_id || !rights ||
        (rights & ~allowed))
        return NUX_E_INVALID;
    if (nux_task_get(owner, &task) != NUX_OK ||
        task.state == NUX_TASK_UNUSED || task.state == NUX_TASK_DYING)
        return NUX_E_ACCESS;

    nux_arch_lock(&cap_lock);
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        nux_capability_entry *entry = &entries[i];
        if (entry->active) continue;
        entry->active = 1;
        entry->owner = owner;
        entry->type = type;
        entry->object_id = object_id;
        entry->rights = rights;
        *out = (entry->generation << 8) | (i + 1);
        nux_arch_unlock(&cap_lock);

        /*
         * Revalidate after publishing the handle. This closes the race where
         * task teardown begins between the initial owner check and insertion.
         */
        if (nux_task_get(owner, &task) != NUX_OK ||
            task.state == NUX_TASK_UNUSED || task.state == NUX_TASK_DYING) {
            (void)nux_capability_revoke(owner, *out);
            *out = 0;
            return NUX_E_ACCESS;
        }
        return NUX_OK;
    }
    nux_arch_unlock(&cap_lock);
    return NUX_E_LIMIT;
}

nux_status nux_capability_check(nux_task_id owner, nux_capability cap,
                                nux_object_type type, nux_rights required,
                                nux_object_id *object_id)
{
    nux_u32 slot = cap & 0xffu;
    nux_u32 generation = cap >> 8;
    if (!owner || !slot || slot > NUX_CAPABILITY_LIMIT || !generation ||
        !object_id || type == NUX_OBJECT_NONE)
        return NUX_E_INVALID;

    nux_arch_lock(&cap_lock);
    nux_capability_entry *entry = &entries[slot - 1];
    if (!entry->active || entry->generation != generation ||
        entry->owner != owner || entry->type != type) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    if ((entry->rights & required) != required) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    *object_id = entry->object_id;
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}

nux_status nux_capability_revoke(nux_task_id owner, nux_capability cap)
{
    nux_u32 slot = cap & 0xffu;
    nux_u32 generation = cap >> 8;
    if (!owner || !slot || slot > NUX_CAPABILITY_LIMIT || !generation)
        return NUX_E_INVALID;

    nux_arch_lock(&cap_lock);
    nux_capability_entry *entry = &entries[slot - 1];
    if (!entry->active || entry->generation != generation) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->owner != owner) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    nux_capability_entry_revoke(entry);
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}

nux_u32 nux_capability_revoke_object(nux_object_type type,
                                     nux_object_id object_id)
{
    nux_u32 revoked = 0;
    if (type <= NUX_OBJECT_NONE || type > NUX_OBJECT_MEMORY || !object_id)
        return 0;
    nux_arch_lock(&cap_lock);
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        nux_capability_entry *entry = &entries[i];
        if (!entry->active || entry->type != type ||
            entry->object_id != object_id)
            continue;
        nux_capability_entry_revoke(entry);
        ++revoked;
    }
    nux_arch_unlock(&cap_lock);
    return revoked;
}

nux_u32 nux_capability_revoke_task(nux_task_id owner)
{
    nux_u32 revoked = 0;
    if (!owner) return 0;
    nux_arch_lock(&cap_lock);
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        nux_capability_entry *entry = &entries[i];
        if (!entry->active || entry->owner != owner) continue;
        nux_capability_entry_revoke(entry);
        ++revoked;
    }
    nux_arch_unlock(&cap_lock);
    return revoked;
}

nux_status nux_capability_consume(nux_task_id owner, nux_capability cap, nux_object_type type,
                                  nux_rights required, nux_object_id *object_id)
{
    nux_u32 slot = cap & 0xffu;
    nux_u32 generation = cap >> 8;
    if (!owner || !slot || slot > NUX_CAPABILITY_LIMIT || !generation ||
        !object_id || type == NUX_OBJECT_NONE || !required)
        return NUX_E_INVALID;
    nux_arch_lock(&cap_lock);
    nux_capability_entry *entry = &entries[slot - 1];
    if (!entry->active || entry->generation != generation ||
        entry->owner != owner || entry->type != type ||
        (entry->rights & required) != required) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    *object_id = entry->object_id;
    entry->active = 0;
    entry->owner = 0;
    entry->object_id = 0;
    entry->rights = 0;
    entry->type = NUX_OBJECT_NONE;
    if (++entry->generation == 0 || entry->generation > 0x00ffffffu)
        entry->generation = 1;
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}

nux_u32 nux_capability_revoke_owner_object_rights(nux_task_id owner,
                                                   nux_object_type type,
                                                   nux_object_id object_id,
                                                   nux_rights rights)
{
    nux_u32 revoked = 0;
    if (!owner || type <= NUX_OBJECT_NONE || type > NUX_OBJECT_MEMORY ||
        !object_id || !rights)
        return 0;
    nux_arch_lock(&cap_lock);
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        nux_capability_entry *entry = &entries[i];
        if (!entry->active || entry->owner != owner || entry->type != type ||
            entry->object_id != object_id || !(entry->rights & rights))
            continue;
        nux_capability_entry_revoke(entry);
        ++revoked;
    }
    nux_arch_unlock(&cap_lock);
    return revoked;
}

nux_status nux_capability_move(nux_task_id source_owner,
                               nux_capability source,
                               nux_task_id destination_owner,
                               nux_object_type type,
                               nux_rights required,
                               nux_rights granted,
                               nux_object_id *object_id,
                               nux_capability *out)
{
    const nux_rights allowed = NUX_RIGHT_READ | NUX_RIGHT_WRITE |
        NUX_RIGHT_SEND | NUX_RIGHT_RECEIVE | NUX_RIGHT_MAP |
        NUX_RIGHT_CONTROL | NUX_RIGHT_TRANSFER | NUX_RIGHT_SEND_ONCE;
    nux_task_info task;
    nux_u32 source_slot = source & 0xffu;
    nux_u32 source_generation = source >> 8;
    nux_u32 destination_slot = NUX_CAPABILITY_LIMIT;

    if (!source_owner || !destination_owner || source_owner == destination_owner ||
        !source_slot || source_slot > NUX_CAPABILITY_LIMIT || !source_generation ||
        type == NUX_OBJECT_NONE || !required || !granted ||
        (granted & ~allowed) || !object_id || !out)
        return NUX_E_INVALID;
    if (nux_task_get(destination_owner, &task) != NUX_OK ||
        task.state == NUX_TASK_UNUSED || task.state == NUX_TASK_DYING)
        return NUX_E_ACCESS;

    nux_arch_lock(&cap_lock);
    nux_capability_entry *source_entry = &entries[source_slot - 1];
    if (!source_entry->active || source_entry->generation != source_generation ||
        source_entry->owner != source_owner || source_entry->type != type ||
        (source_entry->rights & required) != required) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        if (!entries[i].active) {
            destination_slot = i;
            break;
        }
    }
    if (destination_slot == NUX_CAPABILITY_LIMIT) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_LIMIT;
    }

    nux_capability_entry *destination_entry = &entries[destination_slot];
    destination_entry->active = 1;
    destination_entry->owner = destination_owner;
    destination_entry->object_id = source_entry->object_id;
    destination_entry->rights = granted;
    destination_entry->type = type;
    *object_id = source_entry->object_id;
    *out = (destination_entry->generation << 8) | (destination_slot + 1);
    nux_capability_entry_revoke(source_entry);
    nux_arch_unlock(&cap_lock);

    if (nux_task_get(destination_owner, &task) != NUX_OK ||
        task.state == NUX_TASK_UNUSED || task.state == NUX_TASK_DYING) {
        (void)nux_capability_revoke(destination_owner, *out);
        *out = 0;
        return NUX_E_ACCESS;
    }
    return NUX_OK;
}
