#include <nux/port.h>

#define NUX_PORT_MESSAGE_POOL_LIMIT 256u
#define NUX_PORT_SET_WIDTH 64u

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

typedef struct {
    nux_u32 id;
    nux_u32 kind;
    nux_u64 sequence;
    nux_u64 deadline_ns;
    nux_u32 inline_size;
    nux_u32 descriptor_count;
    nux_u8 data[NUX_MESSAGE_INLINE_MAX];
    nux_u8 active;
} nux_port_message_slot;

typedef struct {
    nux_object_id object_id;
    nux_task_id owner;
    nux_capability send_capability;
    nux_capability receive_capability;
    nux_u16 queue[NUX_PORT_QUEUE_DEFAULT];
    nux_u32 queue_limit;
    nux_u32 head;
    nux_u32 tail;
    nux_u32 count;
    nux_u8 active;
} nux_port_entry;

typedef struct {
    nux_port_set_id id;
    nux_task_id owner;
    nux_capability ports[NUX_PORT_SET_WIDTH];
    nux_u32 count;
    nux_u8 active;
} nux_port_set_entry;

static nux_port_entry ports[NUX_PORT_LIMIT];
static nux_port_message_slot messages[NUX_PORT_MESSAGE_POOL_LIMIT];
static nux_port_set_entry port_sets[NUX_PORT_SET_LIMIT];
static volatile nux_u32 port_lock;
static nux_object_id next_port_id;
static nux_port_set_id next_port_set_id;
static nux_u64 next_message_sequence;

static nux_port_entry *nux_port_find(nux_object_id id)
{
    for (nux_u32 i = 0; i < NUX_PORT_LIMIT; ++i)
        if (ports[i].active && ports[i].object_id == id)
            return &ports[i];
    return NUX_NULL;
}

static nux_port_set_entry *nux_port_set_find(nux_port_set_id id)
{
    for (nux_u32 i = 0; i < NUX_PORT_SET_LIMIT; ++i)
        if (port_sets[i].active && port_sets[i].id == id)
            return &port_sets[i];
    return NUX_NULL;
}

static nux_i32 nux_message_slot_alloc(void)
{
    for (nux_u32 i = 0; i < NUX_PORT_MESSAGE_POOL_LIMIT; ++i) {
        if (!messages[i].active) {
            messages[i].active = 1;
            return (nux_i32)i;
        }
    }
    return -1;
}

nux_status nux_port_system_init(void)
{
    nux_arch_lock(&port_lock);
    for (nux_u32 i = 0; i < NUX_PORT_LIMIT; ++i) {
        ports[i].active = 0;
        ports[i].count = 0;
        ports[i].head = 0;
        ports[i].tail = 0;
    }
    for (nux_u32 i = 0; i < NUX_PORT_MESSAGE_POOL_LIMIT; ++i)
        messages[i].active = 0;
    for (nux_u32 i = 0; i < NUX_PORT_SET_LIMIT; ++i) {
        port_sets[i].active = 0;
        port_sets[i].count = 0;
    }
    next_port_id = 1;
    next_port_set_id = 1;
    next_message_sequence = 1;
    nux_arch_unlock(&port_lock);
    return NUX_OK;
}

nux_status nux_port_create(nux_task_id owner, nux_u32 queue_limit,
                           nux_capability *send, nux_capability *receive)
{
    nux_task_info task;
    nux_u32 slot = NUX_PORT_LIMIT;
    nux_object_id object_id;
    nux_capability send_handle, receive_handle;

    if (!owner || !send || !receive) return NUX_E_INVALID;
    if (nux_task_get(owner, &task) != NUX_OK ||
        task.state == NUX_TASK_DYING || task.state == NUX_TASK_UNUSED)
        return NUX_E_ACCESS;
    if (queue_limit == 0) queue_limit = NUX_PORT_QUEUE_DEFAULT;
    if (queue_limit > NUX_PORT_QUEUE_DEFAULT) return NUX_E_LIMIT;

    nux_arch_lock(&port_lock);
    for (nux_u32 i = 0; i < NUX_PORT_LIMIT; ++i) {
        if (!ports[i].active) { slot = i; break; }
    }
    if (slot == NUX_PORT_LIMIT) {
        nux_arch_unlock(&port_lock);
        return NUX_E_LIMIT;
    }
    object_id = next_port_id++;
    if (!object_id) object_id = next_port_id++;
    ports[slot].active = 1;
    ports[slot].object_id = object_id;
    ports[slot].owner = owner;
    ports[slot].queue_limit = queue_limit;
    ports[slot].head = ports[slot].tail = ports[slot].count = 0;
    ports[slot].send_capability = 0;
    ports[slot].receive_capability = 0;
    nux_arch_unlock(&port_lock);


    nux_status status = nux_capability_create(
        owner, NUX_OBJECT_PORT, object_id, NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER,
        &send_handle);
    if (status != NUX_OK) goto rollback;
    status = nux_capability_create(
        owner, NUX_OBJECT_PORT, object_id,
        NUX_RIGHT_RECEIVE | NUX_RIGHT_READ | NUX_RIGHT_CONTROL,
        &receive_handle);
    if (status != NUX_OK) {
        nux_capability_revoke(owner, send_handle);
        goto rollback;
    }

    nux_arch_lock(&port_lock);
    ports[slot].send_capability = send_handle;
    ports[slot].receive_capability = receive_handle;
    nux_arch_unlock(&port_lock);
    *send = send_handle;
    *receive = receive_handle;
    return NUX_OK;

rollback:
    nux_arch_lock(&port_lock);
    ports[slot].active = 0;
    nux_arch_unlock(&port_lock);
    return status;
}

nux_status nux_port_task_destroy(nux_task_id owner)
{
    if (!owner) return NUX_E_INVALID;

    nux_arch_lock(&port_lock);
    for (nux_u32 i = 0; i < NUX_PORT_SET_LIMIT; ++i) {
        if (port_sets[i].active && port_sets[i].owner == owner) {
            port_sets[i].active = 0;
            port_sets[i].count = 0;
        }
    }
    nux_arch_unlock(&port_lock);

    for (;;) {
        nux_object_id object_id = 0;
        nux_u32 found = 0;

        nux_arch_lock(&port_lock);
        for (nux_u32 i = 0; i < NUX_PORT_LIMIT; ++i) {
            nux_port_entry *port = &ports[i];
            if (!port->active || port->owner != owner) continue;
            object_id = port->object_id;
            while (port->count) {
                nux_u16 encoded = port->queue[port->head];
                if (encoded && encoded <= NUX_PORT_MESSAGE_POOL_LIMIT)
                    messages[encoded - 1].active = 0;
                port->queue[port->head] = 0;
                port->head = (port->head + 1) % port->queue_limit;
                --port->count;
            }
            port->active = 0;
            port->object_id = 0;
            port->owner = 0;
            port->send_capability = 0;
            port->receive_capability = 0;
            found = 1;
            break;
        }
        nux_arch_unlock(&port_lock);

        if (!found) break;
        if (object_id) (void)nux_capability_revoke_object(NUX_OBJECT_PORT, object_id);
    }
    (void)nux_capability_revoke_task(owner);
    return NUX_OK;
}

nux_status nux_port_destroy(nux_task_id owner, nux_capability control)
{
    nux_object_id object_id;
    nux_status status = nux_capability_check(
        owner, control, NUX_OBJECT_PORT, NUX_RIGHT_CONTROL, &object_id);
    if (status != NUX_OK) return status;

    nux_arch_lock(&port_lock);
    nux_port_entry *port = nux_port_find(object_id);
    if (!port) {
        nux_arch_unlock(&port_lock);
        return NUX_E_NOT_FOUND;
    }

    while (port->count) {
        nux_u16 encoded = port->queue[port->head];
        if (encoded) messages[encoded - 1].active = 0;
        port->head = (port->head + 1) % port->queue_limit;
        --port->count;
    }
    port->active = 0;
    nux_arch_unlock(&port_lock);

    (void)nux_capability_revoke_object(NUX_OBJECT_PORT, object_id);
    return NUX_OK;
}

nux_status nux_port_send(nux_task_id sender, nux_capability send, const nux_message *message,
                         nux_u64 timeout_ns)
{
    nux_object_id object_id;
    nux_status status;
    if (timeout_ns) return NUX_E_NOT_SUPPORTED;
    if (!message || message->inline_size > NUX_MESSAGE_INLINE_MAX ||
        (message->inline_size && !message->inline_data) ||
        message->descriptor_count != 0 ||
        message->kind > (nux_u32)NUX_PORT_MSG_RPC)
        return NUX_E_INVALID;
    status = nux_capability_check(sender, send, NUX_OBJECT_PORT,
                                  NUX_RIGHT_SEND, &object_id);
    if (status != NUX_OK) return status;

    nux_arch_lock(&port_lock);
    nux_port_entry *port = nux_port_find(object_id);
    if (!port) {
        nux_arch_unlock(&port_lock);
        return NUX_E_NOT_FOUND;
    }
    if (port->count == port->queue_limit) {
        nux_arch_unlock(&port_lock);
        return NUX_E_WOULD_BLOCK;
    }
    nux_i32 index = nux_message_slot_alloc();
    if (index < 0) {
        nux_arch_unlock(&port_lock);
        return NUX_E_WOULD_BLOCK;
    }

    nux_port_message_slot *slot = &messages[index];
    slot->id = message->id;
    slot->kind = message->kind;
    slot->sequence = next_message_sequence++;
    if (!slot->sequence) slot->sequence = next_message_sequence++;
    slot->deadline_ns = message->deadline_ns;
    slot->inline_size = message->inline_size;
    slot->descriptor_count = 0;
    const nux_u8 *source = (const nux_u8 *)message->inline_data;
    for (nux_u32 i = 0; i < message->inline_size; ++i)
        slot->data[i] = source[i];

    port->queue[port->tail] = (nux_u16)(index + 1);
    port->tail = (port->tail + 1) % port->queue_limit;
    ++port->count;
    nux_arch_unlock(&port_lock);
    return NUX_OK;
}

nux_status nux_port_receive(nux_task_id receiver, nux_capability receive, nux_message *message,
                            nux_u64 timeout_ns)
{
    nux_object_id object_id;
    nux_status status;
    void *buffer;
    nux_u32 capacity;
    if (timeout_ns) return NUX_E_NOT_SUPPORTED;
    if (!message) return NUX_E_INVALID;
    buffer = message->inline_data;
    capacity = message->inline_capacity;
    status = nux_capability_check(receiver, receive, NUX_OBJECT_PORT,
                                  NUX_RIGHT_RECEIVE, &object_id);
    if (status != NUX_OK) return status;

    nux_arch_lock(&port_lock);
    nux_port_entry *port = nux_port_find(object_id);
    if (!port) {
        nux_arch_unlock(&port_lock);
        return NUX_E_NOT_FOUND;
    }
    if (!port->count) {
        nux_arch_unlock(&port_lock);
        return NUX_E_WOULD_BLOCK;
    }
    nux_u16 encoded = port->queue[port->head];
    if (!encoded || encoded > NUX_PORT_MESSAGE_POOL_LIMIT) {
        nux_arch_unlock(&port_lock);
        return NUX_E_BAD_STATE;
    }
    nux_port_message_slot *slot = &messages[encoded - 1];
    if (!slot->active) {
        nux_arch_unlock(&port_lock);
        return NUX_E_BAD_STATE;
    }
    if (slot->inline_size > capacity ||
        (slot->inline_size && !buffer)) {
        nux_arch_unlock(&port_lock);
        return NUX_E_LIMIT;
    }

    nux_u8 *destination = (nux_u8 *)buffer;
    for (nux_u32 i = 0; i < slot->inline_size; ++i)
        destination[i] = slot->data[i];
    message->id = slot->id;
    message->kind = slot->kind;
    message->sequence = slot->sequence;
    message->deadline_ns = slot->deadline_ns;
    message->inline_size = slot->inline_size;
    message->descriptor_count = 0;
    message->inline_data = buffer;
    message->inline_capacity = capacity;

    slot->active = 0;
    port->queue[port->head] = 0;
    port->head = (port->head + 1) % port->queue_limit;
    --port->count;
    nux_arch_unlock(&port_lock);
    return NUX_OK;
}

nux_status nux_port_move_right(nux_task_id source_owner,
                               nux_capability source,
                               nux_task_id destination,
                               nux_port_right_kind right,
                               nux_capability *out)
{
    nux_task_info destination_task;
    nux_object_id object_id;
    nux_rights required;
    nux_rights granted;

    if (!source_owner || !destination || source_owner == destination ||
        !out || right < NUX_PORT_RIGHT_SEND ||
        right > NUX_PORT_RIGHT_SEND_ONCE)
        return NUX_E_INVALID;
    if (nux_task_get(destination, &destination_task) != NUX_OK ||
        destination_task.state == NUX_TASK_DYING ||
        destination_task.state == NUX_TASK_UNUSED)
        return NUX_E_ACCESS;

    if (right == NUX_PORT_RIGHT_SEND) {
        required = NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER;
        granted = NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER;
    } else if (right == NUX_PORT_RIGHT_RECEIVE) {
        required = NUX_RIGHT_RECEIVE | NUX_RIGHT_CONTROL | NUX_RIGHT_TRANSFER;
        granted = NUX_RIGHT_RECEIVE | NUX_RIGHT_READ |
            NUX_RIGHT_CONTROL | NUX_RIGHT_TRANSFER;
    } else {
        return NUX_E_NOT_SUPPORTED;
    }

    nux_status status = nux_capability_check(
        source_owner, source, NUX_OBJECT_PORT, required, &object_id);
    if (status != NUX_OK) return status;
    status = nux_capability_create(destination, NUX_OBJECT_PORT,
                                   object_id, granted, out);
    if (status != NUX_OK) return status;
    status = nux_capability_revoke(source_owner, source);
    if (status != NUX_OK) {
        (void)nux_capability_revoke(destination, *out);
        *out = 0;
        return status;
    }
    return NUX_OK;
}

nux_status nux_port_set_create(nux_task_id owner, nux_port_set_id *out)
{
    nux_task_info task;
    if (!owner || !out) return NUX_E_INVALID;
    if (nux_task_get(owner, &task) != NUX_OK) return NUX_E_NOT_FOUND;
    nux_arch_lock(&port_lock);
    for (nux_u32 i = 0; i < NUX_PORT_SET_LIMIT; ++i) {
        if (port_sets[i].active) continue;
        nux_port_set_entry *set = &port_sets[i];
        set->active = 1;
        set->owner = owner;
        set->count = 0;
        set->id = next_port_set_id++;
        if (!set->id) set->id = next_port_set_id++;
        *out = set->id;
        nux_arch_unlock(&port_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&port_lock);
    return NUX_E_LIMIT;
}

nux_status nux_port_set_add(nux_task_id owner, nux_port_set_id id,
                             nux_capability receive)
{
    nux_object_id object_id;
    nux_status status;
    if (!owner) return NUX_E_INVALID;

    nux_arch_lock(&port_lock);
    nux_port_set_entry *set = nux_port_set_find(id);
    if (!set) {
        nux_arch_unlock(&port_lock);
        return NUX_E_NOT_FOUND;
    }
    if (set->owner != owner) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
    }
    nux_arch_unlock(&port_lock);

    status = nux_capability_check(owner, receive, NUX_OBJECT_PORT,
                                  NUX_RIGHT_RECEIVE, &object_id);
    if (status != NUX_OK) return status;
    (void)object_id;

    nux_arch_lock(&port_lock);
    set = nux_port_set_find(id);
    if (!set || set->owner != owner) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
    }
    for (nux_u32 i = 0; i < set->count; ++i) {
        if (set->ports[i] == receive) {
            nux_arch_unlock(&port_lock);
            return NUX_OK;
        }
    }
    if (set->count == NUX_PORT_SET_WIDTH) {
        nux_arch_unlock(&port_lock);
        return NUX_E_LIMIT;
    }
    set->ports[set->count++] = receive;
    nux_arch_unlock(&port_lock);
    return NUX_OK;
}

nux_status nux_port_set_remove(nux_task_id owner, nux_port_set_id id,
                                nux_capability receive)
{
    nux_arch_lock(&port_lock);
    nux_port_set_entry *set = nux_port_set_find(id);
    if (!set) {
        nux_arch_unlock(&port_lock);
        return NUX_E_NOT_FOUND;
    }
    if (set->owner != owner) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
    }
    for (nux_u32 i = 0; i < set->count; ++i) {
        if (set->ports[i] != receive) continue;
        for (nux_u32 j = i + 1; j < set->count; ++j)
            set->ports[j - 1] = set->ports[j];
        --set->count;
        nux_arch_unlock(&port_lock);
        return NUX_OK;
    }
    nux_arch_unlock(&port_lock);
    return NUX_E_NOT_FOUND;
}

nux_status nux_port_set_receive(nux_task_id owner, nux_port_set_id id, nux_message *message,
                                nux_u64 timeout_ns)
{
    nux_capability snapshot[NUX_PORT_SET_WIDTH];
    nux_u32 count;
    if (!owner || !message) return NUX_E_INVALID;
    nux_arch_lock(&port_lock);
    nux_port_set_entry *set = nux_port_set_find(id);
    if (!set) {
        nux_arch_unlock(&port_lock);
        return NUX_E_NOT_FOUND;
    }
    if (set->owner != owner) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
    }
    count = set->count;
    for (nux_u32 i = 0; i < count; ++i) snapshot[i] = set->ports[i];
    nux_arch_unlock(&port_lock);

    for (nux_u32 i = 0; i < count; ++i) {
        nux_status status = nux_port_receive(owner, snapshot[i], message, 0);
        if (status == NUX_OK) return NUX_OK;
        if (status != NUX_E_WOULD_BLOCK && status != NUX_E_NOT_FOUND)
            return status;
    }
    return timeout_ns ? NUX_E_NOT_SUPPORTED : NUX_E_WOULD_BLOCK;
}

nux_status nux_port_request_notification(nux_task_id owner, nux_capability receive, nux_u32 event,
                                          nux_capability notify_port)
{
    nux_object_id object_id, notify_id;
    nux_status status = nux_capability_check(
        owner, receive, NUX_OBJECT_PORT, NUX_RIGHT_CONTROL, &object_id);
    if (status != NUX_OK) return status;
    status = nux_capability_check(
        owner, notify_port, NUX_OBJECT_PORT, NUX_RIGHT_SEND, &notify_id);
    if (status != NUX_OK) return status;
    (void)object_id;
    (void)notify_id;
    (void)event;
    return NUX_E_NOT_SUPPORTED;
}
