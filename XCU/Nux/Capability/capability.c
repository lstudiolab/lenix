#include <nux/capability.h>
#include <nux/task.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

typedef struct {
    nux_u32 generation;
    nux_object_id object_id;
    nux_rights rights;
    nux_object_type type;
    nux_u8 state; /* 0 free, 1 live, 2 pending */
} nux_capability_entry;

typedef struct {
    nux_task_id owner;
    nux_u8 active;
    nux_capability_entry entries[NUX_CAPABILITY_LIMIT];
} nux_capability_space;

static nux_capability_space spaces[NUX_TASK_LIMIT];
static volatile nux_u32 cap_lock;

static void entry_revoke(nux_capability_entry *entry)
{
    entry->state = 0;
    entry->object_id = 0;
    entry->rights = 0;
    entry->type = NUX_OBJECT_NONE;
    if (++entry->generation == 0 || entry->generation > 0x00ffffffu)
        entry->generation = 1;
}

static nux_capability_space *space_find(nux_task_id owner)
{
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i)
        if (spaces[i].active && spaces[i].owner == owner)
            return &spaces[i];
    return NUX_NULL;
}

static nux_capability_space *space_allocate(nux_task_id owner)
{
    nux_capability_space *space = space_find(owner);
    if (space) return space;
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        space = &spaces[i];
        if (space->active) continue;
        space->owner = owner;
        space->active = 1;
        for (nux_u32 j = 0; j < NUX_CAPABILITY_LIMIT; ++j) {
            nux_capability_entry *entry = &space->entries[j];
            if (!entry->generation) entry->generation = 1;
            entry->state = 0;
            entry->object_id = 0;
            entry->rights = 0;
            entry->type = NUX_OBJECT_NONE;
        }
        return space;
    }
    return NUX_NULL;
}

static nux_capability_entry *entry_from_handle(nux_capability_space *space,
                                                nux_capability cap,
                                                nux_u32 *slot_out,
                                                nux_u32 *generation_out)
{
    nux_u32 slot = cap & 0xffu;
    nux_u32 generation = cap >> 8;
    if (!space || !slot || slot > NUX_CAPABILITY_LIMIT || !generation)
        return NUX_NULL;
    if (slot_out) *slot_out = slot - 1;
    if (generation_out) *generation_out = generation;
    return &space->entries[slot - 1];
}


static nux_status task_is_live(nux_task_id owner)
{
    nux_task_info task;
    if (!owner || nux_task_get(owner, &task) != NUX_OK ||
        task.state == NUX_TASK_UNUSED || task.state == NUX_TASK_DYING)
        return NUX_E_ACCESS;
    return NUX_OK;
}

nux_status nux_capability_system_init(void)
{
    nux_arch_lock(&cap_lock);
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        spaces[i].owner = 0;
        spaces[i].active = 0;
        for (nux_u32 j = 0; j < NUX_CAPABILITY_LIMIT; ++j) {
            spaces[i].entries[j].generation = 1;
            spaces[i].entries[j].state = 0;
            spaces[i].entries[j].object_id = 0;
            spaces[i].entries[j].rights = 0;
            spaces[i].entries[j].type = NUX_OBJECT_NONE;
        }
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
    if (!out || type <= NUX_OBJECT_NONE || type > NUX_OBJECT_MEMORY ||
        !object_id || !rights || (rights & ~allowed))
        return NUX_E_INVALID;
    if (task_is_live(owner) != NUX_OK) return NUX_E_ACCESS;

    nux_u32 slot = NUX_CAPABILITY_LIMIT;
    nux_u32 generation = 0;
    nux_arch_lock(&cap_lock);
    nux_capability_space *space = space_allocate(owner);
    if (!space) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_LIMIT;
    }
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        nux_capability_entry *entry = &space->entries[i];
        if (entry->state) continue;
        slot = i;
        generation = entry->generation;
        entry->state = 2;
        entry->type = type;
        entry->object_id = object_id;
        entry->rights = rights;
        *out = (generation << 8) | (slot + 1);
        break;
    }
    nux_arch_unlock(&cap_lock);
    if (slot == NUX_CAPABILITY_LIMIT) return NUX_E_LIMIT;

    if (task_is_live(owner) != NUX_OK) {
        nux_arch_lock(&cap_lock);
        space = space_find(owner);
        if (space) {
            nux_capability_entry *entry = &space->entries[slot];
            if (entry->state == 2 && entry->generation == generation)
                entry_revoke(entry);
        }
        nux_arch_unlock(&cap_lock);
        *out = 0;
        return NUX_E_ACCESS;
    }

    nux_arch_lock(&cap_lock);
    space = space_find(owner);
    if (!space || space->entries[slot].state != 2 ||
        space->entries[slot].generation != generation) {
        nux_arch_unlock(&cap_lock);
        *out = 0;
        return NUX_E_ACCESS;
    }
    space->entries[slot].state = 1;
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}

nux_status nux_capability_check(nux_task_id owner, nux_capability cap,
                                nux_object_type type, nux_rights required,
                                nux_object_id *object_id)
{
    if (!owner || !object_id || type == NUX_OBJECT_NONE || !required) return NUX_E_INVALID;
    nux_arch_lock(&cap_lock);
    nux_capability_space *space = space_find(owner);
    nux_u32 generation;
    nux_capability_entry *entry = entry_from_handle(space, cap, NUX_NULL,
                                                     &generation);
    if (!entry || entry->state != 1 || entry->generation != generation ||
        entry->type != type || (entry->rights & required) != required) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    *object_id = entry->object_id;
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}

nux_status nux_capability_revoke(nux_task_id owner, nux_capability cap)
{
    if (!owner) return NUX_E_INVALID;
    nux_arch_lock(&cap_lock);
    nux_capability_space *space = space_find(owner);
    nux_u32 generation;
    nux_capability_entry *entry = entry_from_handle(space, cap, NUX_NULL,
                                                     &generation);
    if (!entry || entry->state != 1 || entry->generation != generation) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_NOT_FOUND;
    }
    entry_revoke(entry);
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
    for (nux_u32 i = 0; i < NUX_TASK_LIMIT; ++i) {
        if (!spaces[i].active) continue;
        for (nux_u32 j = 0; j < NUX_CAPABILITY_LIMIT; ++j) {
            nux_capability_entry *entry = &spaces[i].entries[j];
            if (!entry->state || entry->type != type ||
                entry->object_id != object_id)
                continue;
            entry_revoke(entry);
            ++revoked;
        }
    }
    nux_arch_unlock(&cap_lock);
    return revoked;
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
    nux_capability_space *space = space_find(owner);
    if (space) {
        for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
            nux_capability_entry *entry = &space->entries[i];
            if (!entry->state || entry->type != type ||
                entry->object_id != object_id || !(entry->rights & rights))
                continue;
            entry_revoke(entry);
            ++revoked;
        }
    }
    nux_arch_unlock(&cap_lock);
    return revoked;
}

nux_u32 nux_capability_revoke_task(nux_task_id owner)
{
    if (!owner) return 0;
    nux_u32 revoked = 0;
    nux_arch_lock(&cap_lock);
    nux_capability_space *space = space_find(owner);
    if (space) {
        for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
            if (!space->entries[i].state) continue;
            entry_revoke(&space->entries[i]);
            ++revoked;
        }
        space->owner = 0;
        space->active = 0;
    }
    nux_arch_unlock(&cap_lock);
    return revoked;
}

nux_status nux_capability_consume(nux_task_id owner, nux_capability cap,
                                  nux_object_type type, nux_rights required,
                                  nux_object_id *object_id)
{
    if (!owner || !object_id || type == NUX_OBJECT_NONE || !required)
        return NUX_E_INVALID;
    nux_arch_lock(&cap_lock);
    nux_capability_space *space = space_find(owner);
    nux_u32 generation;
    nux_capability_entry *entry = entry_from_handle(space, cap, NUX_NULL,
                                                     &generation);
    if (!entry || entry->state != 1 || entry->generation != generation ||
        entry->type != type || (entry->rights & required) != required) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    *object_id = entry->object_id;
    entry_revoke(entry);
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}


nux_status nux_capability_derive(nux_task_id owner, nux_capability source,
                                 nux_rights required, nux_rights granted,
                                 nux_capability *out)
{
    if (!owner || !required || !granted || !out)
        return NUX_E_INVALID;
    /* Receive and control rights are unique; they move, they do not derive. */
    if (granted & (NUX_RIGHT_RECEIVE | NUX_RIGHT_CONTROL))
        return NUX_E_ACCESS;
    if (task_is_live(owner) != NUX_OK) return NUX_E_ACCESS;

    nux_u32 source_generation, destination_slot = NUX_CAPABILITY_LIMIT;
    nux_u32 destination_generation = 0;
    nux_object_id object_id = 0;
    nux_object_type type = NUX_OBJECT_NONE;
    nux_arch_lock(&cap_lock);
    nux_capability_space *space = space_find(owner);
    nux_capability_entry *source_entry = entry_from_handle(
        space, source, NUX_NULL, &source_generation);
    if (!source_entry || source_entry->state != 1 ||
        source_entry->generation != source_generation ||
        (source_entry->rights & required) != required ||
        (granted & ~source_entry->rights)) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        if (space->entries[i].state) continue;
        destination_slot = i;
        break;
    }
    if (destination_slot == NUX_CAPABILITY_LIMIT) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_LIMIT;
    }
    nux_capability_entry *destination = &space->entries[destination_slot];
    destination_generation = destination->generation;
    object_id = source_entry->object_id;
    type = source_entry->type;
    destination->state = 2;
    destination->type = type;
    destination->object_id = object_id;
    destination->rights = granted;
    *out = (destination_generation << 8) | (destination_slot + 1);
    nux_arch_unlock(&cap_lock);

    if (task_is_live(owner) != NUX_OK) {
        nux_arch_lock(&cap_lock);
        space = space_find(owner);
        if (space) {
            destination = &space->entries[destination_slot];
            if (destination->state == 2 &&
                destination->generation == destination_generation)
                entry_revoke(destination);
        }
        nux_arch_unlock(&cap_lock);
        *out = 0;
        return NUX_E_ACCESS;
    }

    nux_arch_lock(&cap_lock);
    space = space_find(owner);
    destination = space ? &space->entries[destination_slot] : NUX_NULL;
    if (!destination || destination->state != 2 ||
        destination->generation != destination_generation) {
        nux_arch_unlock(&cap_lock);
        *out = 0;
        return NUX_E_ACCESS;
    }
    destination->state = 1;
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
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
    if (!source_owner || !destination_owner || source_owner == destination_owner ||
        type == NUX_OBJECT_NONE || !required || !granted ||
        (granted & ~allowed) || !object_id || !out)
        return NUX_E_INVALID;
    if (task_is_live(destination_owner) != NUX_OK) return NUX_E_ACCESS;

    nux_u32 source_slot, source_generation, destination_slot = NUX_CAPABILITY_LIMIT;
    nux_u32 destination_generation = 0;
    nux_object_id moved_object = 0;
    nux_arch_lock(&cap_lock);
    nux_capability_space *source_space = space_find(source_owner);
    nux_capability_entry *source_entry = entry_from_handle(
        source_space, source, &source_slot, &source_generation);
    if (!source_entry || source_entry->state != 1 ||
        source_entry->generation != source_generation ||
        source_entry->type != type ||
        (source_entry->rights & required) != required ||
        (granted & ~source_entry->rights)) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_ACCESS;
    }
    nux_capability_space *destination_space = space_allocate(destination_owner);
    if (!destination_space) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_LIMIT;
    }
    for (nux_u32 i = 0; i < NUX_CAPABILITY_LIMIT; ++i) {
        if (destination_space->entries[i].state) continue;
        destination_slot = i;
        break;
    }
    if (destination_slot == NUX_CAPABILITY_LIMIT) {
        nux_arch_unlock(&cap_lock);
        return NUX_E_LIMIT;
    }
    nux_capability_entry *destination_entry =
        &destination_space->entries[destination_slot];
    destination_generation = destination_entry->generation;
    moved_object = source_entry->object_id;
    destination_entry->state = 2;
    destination_entry->type = type;
    destination_entry->object_id = moved_object;
    destination_entry->rights = granted;
    *out = (destination_generation << 8) | (destination_slot + 1);
    nux_arch_unlock(&cap_lock);

    if (task_is_live(destination_owner) != NUX_OK) {
        nux_arch_lock(&cap_lock);
        destination_space = space_find(destination_owner);
        if (destination_space) {
            destination_entry = &destination_space->entries[destination_slot];
            if (destination_entry->state == 2 &&
                destination_entry->generation == destination_generation)
                entry_revoke(destination_entry);
        }
        nux_arch_unlock(&cap_lock);
        *out = 0;
        return NUX_E_ACCESS;
    }

    nux_arch_lock(&cap_lock);
    source_space = space_find(source_owner);
    destination_space = space_find(destination_owner);
    source_entry = entry_from_handle(source_space, source, NUX_NULL,
                                     &source_generation);
    destination_entry = destination_space
        ? &destination_space->entries[destination_slot] : NUX_NULL;
    if (!source_entry || source_entry->state != 1 ||
        source_entry->generation != source_generation ||
        !destination_entry || destination_entry->state != 2 ||
        destination_entry->generation != destination_generation) {
        if (destination_entry && destination_entry->state == 2 &&
            destination_entry->generation == destination_generation)
            entry_revoke(destination_entry);
        nux_arch_unlock(&cap_lock);
        *out = 0;
        return NUX_E_ACCESS;
    }
    destination_entry->state = 1;
    entry_revoke(source_entry);
    *object_id = moved_object;
    nux_arch_unlock(&cap_lock);
    return NUX_OK;
}
