#include <nux/bootstrap.h>
#include <nux/thread.h>
#include <nux/object.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

static nux_service_entry services[NUX_BOOTSTRAP_SERVICE_LIMIT];
static volatile nux_u32 bootstrap_lock;

static nux_size service_name_length(const char *name)
{
    nux_size length = 0;
    if (!name) return 0;
    while (length < NUX_SERVICE_NAME_MAX && name[length]) ++length;
    return length;
}

static nux_u32 service_name_equal(const char *left, const char *right,
                                  nux_size length)
{
    for (nux_size i = 0; i < length; ++i)
        if (left[i] != right[i]) return 0;
    return left[length] == '\0' && right[length] == '\0';
}

static nux_service_entry *service_find(const char *name, nux_size length)
{
    for (nux_u32 i = 0; i < NUX_BOOTSTRAP_SERVICE_LIMIT; ++i)
        if (services[i].service_port &&
            service_name_equal(services[i].name, name, length))
            return &services[i];
    return NUX_NULL;
}

nux_status nux_bootstrap_system_init(void)
{
    nux_arch_lock(&bootstrap_lock);
    for (nux_u32 i = 0; i < NUX_BOOTSTRAP_SERVICE_LIMIT; ++i) {
        services[i].name[0] = '\0';
        services[i].service_port = 0;
        services[i].owner_task = 0;
        services[i].flags = 0;
    }
    nux_arch_unlock(&bootstrap_lock);
    return NUX_OK;
}

nux_status nux_bootstrap_register(const char *name, nux_capability service_port)
{
    nux_task_id owner = nux_thread_current_task();
    nux_object_id object_id = 0;
    nux_size length = service_name_length(name);
    if (!owner || !length || length >= NUX_SERVICE_NAME_MAX || !service_port)
        return NUX_E_INVALID;
    nux_status status = nux_capability_check(owner, service_port,
        NUX_OBJECT_PORT, NUX_RIGHT_SEND, &object_id);
    if (status != NUX_OK) return status;

    nux_arch_lock(&bootstrap_lock);
    if (service_find(name, length)) {
        nux_arch_unlock(&bootstrap_lock);
        return NUX_E_BUSY;
    }
    for (nux_u32 i = 0; i < NUX_BOOTSTRAP_SERVICE_LIMIT; ++i) {
        nux_service_entry *entry = &services[i];
        if (entry->service_port) continue;
        for (nux_size j = 0; j < length; ++j) entry->name[j] = name[j];
        entry->name[length] = '\0';
        entry->service_port = service_port;
        entry->owner_task = owner;
        entry->flags = 0;
        nux_arch_unlock(&bootstrap_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&bootstrap_lock);
    return NUX_E_LIMIT;
}

nux_status nux_bootstrap_lookup(const char *name, nux_capability *service_port)
{
    nux_task_id caller = nux_thread_current_task();
    nux_size length = service_name_length(name);
    if (!caller || !length || length >= NUX_SERVICE_NAME_MAX || !service_port)
        return NUX_E_INVALID;
    *service_port = 0;

    nux_arch_lock(&bootstrap_lock);
    nux_service_entry *entry = service_find(name, length);
    if (!entry) {
        nux_arch_unlock(&bootstrap_lock);
        return NUX_E_NOT_FOUND;
    }
    nux_task_id owner = entry->owner_task;
    nux_capability source = entry->service_port;
    nux_arch_unlock(&bootstrap_lock);

    nux_object_id object_id = 0;
    nux_status status = nux_capability_check(owner, source, NUX_OBJECT_PORT,
                                              NUX_RIGHT_SEND, &object_id);
    if (status != NUX_OK) return status;
    return nux_capability_create(caller, NUX_OBJECT_PORT, object_id,
                                 NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER,
                                 service_port);
}

nux_status nux_bootstrap_unregister(const char *name)
{
    nux_task_id caller = nux_thread_current_task();
    nux_size length = service_name_length(name);
    if (!caller || !length || length >= NUX_SERVICE_NAME_MAX)
        return NUX_E_INVALID;
    nux_arch_lock(&bootstrap_lock);
    nux_service_entry *entry = service_find(name, length);
    if (!entry) {
        nux_arch_unlock(&bootstrap_lock);
        return NUX_E_NOT_FOUND;
    }
    if (entry->owner_task != caller) {
        nux_arch_unlock(&bootstrap_lock);
        return NUX_E_ACCESS;
    }
    entry->name[0] = '\0';
    entry->service_port = 0;
    entry->owner_task = 0;
    entry->flags = 0;
    nux_arch_unlock(&bootstrap_lock);
    return NUX_OK;
}

void nux_bootstrap_task_destroy(nux_task_id owner)
{
    if (!owner) return;
    nux_arch_lock(&bootstrap_lock);
    for (nux_u32 i = 0; i < NUX_BOOTSTRAP_SERVICE_LIMIT; ++i) {
        if (services[i].owner_task != owner) continue;
        services[i].name[0] = '\0';
        services[i].service_port = 0;
        services[i].owner_task = 0;
        services[i].flags = 0;
    }
    nux_arch_unlock(&bootstrap_lock);
}
