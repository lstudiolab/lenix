#include <nux/sync.h>

#define NUX_SYNC_LOCK_LIMIT 64u

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

typedef struct {
    nux_semaphore_id id;
    nux_u32 count;
    nux_u32 maximum;
    nux_u32 waiters;
    nux_u8 active;
} nux_semaphore_entry;

typedef struct {
    nux_lock_set_id id;
    nux_u32 lock_count;
    nux_u32 held[NUX_SYNC_LOCK_LIMIT];
    nux_u8 active;
} nux_lock_set_entry;

typedef struct {
    nux_condition_id id;
    nux_u64 sequence;
    nux_u8 active;
} nux_condition_entry;

static nux_semaphore_entry semaphores[NUX_SYNC_OBJECT_LIMIT];
static nux_lock_set_entry lock_sets[NUX_SYNC_OBJECT_LIMIT];
static nux_condition_entry conditions[NUX_SYNC_OBJECT_LIMIT];
static volatile nux_u32 sync_lock;
static nux_u32 next_semaphore_id;
static nux_u32 next_lock_set_id;
static nux_u32 next_condition_id;

static nux_semaphore_entry *nux_semaphore_find(nux_semaphore_id id)
{
    for (nux_u32 i = 0; i < NUX_SYNC_OBJECT_LIMIT; ++i)
        if (semaphores[i].active && semaphores[i].id == id)
            return &semaphores[i];
    return NUX_NULL;
}

static nux_lock_set_entry *nux_lock_set_find(nux_lock_set_id id)
{
    for (nux_u32 i = 0; i < NUX_SYNC_OBJECT_LIMIT; ++i)
        if (lock_sets[i].active && lock_sets[i].id == id)
            return &lock_sets[i];
    return NUX_NULL;
}

static nux_condition_entry *nux_condition_find(nux_condition_id id)
{
    for (nux_u32 i = 0; i < NUX_SYNC_OBJECT_LIMIT; ++i)
        if (conditions[i].active && conditions[i].id == id)
            return &conditions[i];
    return NUX_NULL;
}

nux_status nux_sync_system_init(void)
{
    nux_arch_lock(&sync_lock);
    for (nux_u32 i = 0; i < NUX_SYNC_OBJECT_LIMIT; ++i) {
        semaphores[i].active = 0;
        lock_sets[i].active = 0;
        conditions[i].active = 0;
    }
    next_semaphore_id = 1;
    next_lock_set_id = 1;
    next_condition_id = 1;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}

nux_status nux_semaphore_create(nux_u32 initial_count, nux_u32 maximum_count,
                                nux_semaphore_id *out)
{
    if (!out || !maximum_count || initial_count > maximum_count)
        return NUX_E_INVALID;
    nux_arch_lock(&sync_lock);
    for (nux_u32 i = 0; i < NUX_SYNC_OBJECT_LIMIT; ++i) {
        if (semaphores[i].active) continue;
        nux_semaphore_entry *entry = &semaphores[i];
        entry->id = next_semaphore_id++;
        if (!entry->id) entry->id = next_semaphore_id++;
        entry->count = initial_count;
        entry->maximum = maximum_count;
        entry->waiters = 0;
        entry->active = 1;
        *out = entry->id;
        nux_arch_unlock(&sync_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&sync_lock);
    return NUX_E_LIMIT;
}

nux_status nux_semaphore_wait(nux_semaphore_id id, nux_u64 timeout_ns)
{
    if (timeout_ns) return NUX_E_NOT_SUPPORTED;
    nux_arch_lock(&sync_lock);
    nux_semaphore_entry *entry = nux_semaphore_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    if (!entry->count) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_WOULD_BLOCK;
    }
    --entry->count;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}

nux_status nux_semaphore_post(nux_semaphore_id id)
{
    nux_arch_lock(&sync_lock);
    nux_semaphore_entry *entry = nux_semaphore_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->count == entry->maximum) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_LIMIT;
    }
    ++entry->count;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}

nux_status nux_semaphore_post_all(nux_semaphore_id id)
{
    nux_arch_lock(&sync_lock);
    nux_semaphore_entry *entry = nux_semaphore_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    nux_arch_unlock(&sync_lock);
    return NUX_E_NOT_SUPPORTED;
}

nux_status nux_semaphore_destroy(nux_semaphore_id id)
{
    nux_arch_lock(&sync_lock);
    nux_semaphore_entry *entry = nux_semaphore_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->waiters) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_BUSY;
    }
    entry->active = 0;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}

nux_status nux_lock_set_create(nux_u32 lock_count, nux_lock_set_id *out)
{
    if (!out || !lock_count || lock_count > NUX_SYNC_LOCK_LIMIT)
        return NUX_E_INVALID;
    nux_arch_lock(&sync_lock);
    for (nux_u32 i = 0; i < NUX_SYNC_OBJECT_LIMIT; ++i) {
        if (lock_sets[i].active) continue;
        nux_lock_set_entry *entry = &lock_sets[i];
        entry->id = next_lock_set_id++;
        if (!entry->id) entry->id = next_lock_set_id++;
        entry->lock_count = lock_count;
        for (nux_u32 j = 0; j < lock_count; ++j) entry->held[j] = 0;
        entry->active = 1;
        *out = entry->id;
        nux_arch_unlock(&sync_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&sync_lock);
    return NUX_E_LIMIT;
}

nux_status nux_lock_acquire(nux_lock_set_id id, nux_u32 index,
                            nux_u64 timeout_ns)
{
    if (timeout_ns) return NUX_E_NOT_SUPPORTED;
    nux_arch_lock(&sync_lock);
    nux_lock_set_entry *entry = nux_lock_set_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    if (index >= entry->lock_count) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_INVALID;
    }
    if (entry->held[index]) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_WOULD_BLOCK;
    }
    entry->held[index] = 1;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}

nux_status nux_lock_release(nux_lock_set_id id, nux_u32 index)
{
    nux_arch_lock(&sync_lock);
    nux_lock_set_entry *entry = nux_lock_set_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    if (index >= entry->lock_count) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_INVALID;
    }
    if (!entry->held[index]) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_BAD_STATE;
    }
    entry->held[index] = 0;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}

nux_status nux_lock_set_destroy(nux_lock_set_id id)
{
    nux_arch_lock(&sync_lock);
    nux_lock_set_entry *entry = nux_lock_set_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    for (nux_u32 i = 0; i < entry->lock_count; ++i) {
        if (entry->held[i]) {
            nux_arch_unlock(&sync_lock);
            return NUX_E_BUSY;
        }
    }
    entry->active = 0;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}

nux_status nux_condition_create(nux_condition_id *out)
{
    if (!out) return NUX_E_INVALID;
    nux_arch_lock(&sync_lock);
    for (nux_u32 i = 0; i < NUX_SYNC_OBJECT_LIMIT; ++i) {
        if (conditions[i].active) continue;
        nux_condition_entry *entry = &conditions[i];
        entry->id = next_condition_id++;
        if (!entry->id) entry->id = next_condition_id++;
        entry->sequence = 0;
        entry->active = 1;
        *out = entry->id;
        nux_arch_unlock(&sync_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&sync_lock);
    return NUX_E_LIMIT;
}

nux_status nux_condition_wait(nux_condition_id id, nux_u32 mutex_id,
                              nux_u64 timeout_ns)
{
    nux_arch_lock(&sync_lock);
    nux_condition_entry *entry = nux_condition_find(id);
    nux_arch_unlock(&sync_lock);
    if (!entry) return NUX_E_NOT_FOUND;
    (void)mutex_id;
    (void)timeout_ns;
    return NUX_E_NOT_SUPPORTED;
}

nux_status nux_condition_signal(nux_condition_id id)
{
    nux_arch_lock(&sync_lock);
    nux_condition_entry *entry = nux_condition_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    ++entry->sequence;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}

nux_status nux_condition_broadcast(nux_condition_id id)
{
    return nux_condition_signal(id);
}

nux_status nux_condition_destroy(nux_condition_id id)
{
    nux_arch_lock(&sync_lock);
    nux_condition_entry *entry = nux_condition_find(id);
    if (!entry) {
        nux_arch_unlock(&sync_lock);
        return NUX_E_NOT_FOUND;
    }
    entry->active = 0;
    nux_arch_unlock(&sync_lock);
    return NUX_OK;
}
