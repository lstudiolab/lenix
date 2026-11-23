#include <nux/port.h>
#include <nux/thread.h>

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
    nux_thread_id receiver_waiter;
    nux_u8 active;
} nux_port_entry;

typedef struct {
    nux_port_set_id id;
    nux_task_id owner;
    nux_capability ports[NUX_PORT_SET_WIDTH];
    nux_object_id object_ids[NUX_PORT_SET_WIDTH];
    nux_thread_id receiver_waiter;
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
        ports[i].receiver_waiter = 0;
        ports[i].head = 0;
        ports[i].tail = 0;
    }
    for (nux_u32 i = 0; i < NUX_PORT_MESSAGE_POOL_LIMIT; ++i)
        messages[i].active = 0;
    for (nux_u32 i = 0; i < NUX_PORT_SET_LIMIT; ++i) {
        port_sets[i].active = 0;
        port_sets[i].count = 0;
        port_sets[i].receiver_waiter = 0;
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
    ports[slot].receiver_waiter = 0;
    ports[slot].send_capability = 0;
    ports[slot].receive_capability = 0;
    nux_arch_unlock(&port_lock);


    nux_status status = nux_capability_create(
        owner, NUX_OBJECT_PORT, object_id, NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER,
        &send_handle);
    if (status != NUX_OK) goto rollback;
    status = nux_capability_create(
        owner, NUX_OBJECT_PORT, object_id,
        NUX_RIGHT_RECEIVE | NUX_RIGHT_READ | NUX_RIGHT_CONTROL | NUX_RIGHT_TRANSFER,
        &receive_handle);
    if (status != NUX_OK) {
        nux_capability_revoke(owner, send_handle);
        goto rollback;
    }

    nux_arch_lock(&port_lock);
    if (!ports[slot].active || ports[slot].object_id != object_id ||
        ports[slot].owner != owner) {
        nux_arch_unlock(&port_lock);
        (void)nux_capability_revoke_object(NUX_OBJECT_PORT, object_id);
        return NUX_E_BAD_STATE;
    }
    ports[slot].send_capability = send_handle;
    ports[slot].receive_capability = receive_handle;
    nux_arch_unlock(&port_lock);
    *send = send_handle;
    *receive = receive_handle;
    return NUX_OK;

rollback:
    nux_arch_lock(&port_lock);
    if (ports[slot].active && ports[slot].object_id == object_id)
        ports[slot].active = 0;
    nux_arch_unlock(&port_lock);
    (void)nux_capability_revoke_object(NUX_OBJECT_PORT, object_id);
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
        nux_thread_id waiter = 0;
        nux_u32 found = 0;

        nux_arch_lock(&port_lock);
        for (nux_u32 i = 0; i < NUX_PORT_LIMIT; ++i) {
            nux_port_entry *port = &ports[i];
            if (!port->active || port->owner != owner) continue;
            object_id = port->object_id;
            waiter = port->receiver_waiter;
            port->receiver_waiter = 0;
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
        if (waiter) (void)nux_thread_wake(waiter);
        if (object_id) (void)nux_capability_revoke_object(NUX_OBJECT_PORT, object_id);
    }
    (void)nux_capability_revoke_task(owner);
    return NUX_OK;
}

static nux_status nux_port_derive_send(nux_task_id owner,
                                        nux_capability receive,
                                        nux_rights rights,
                                        nux_capability *out)
{
    nux_object_id object_id;
    if (!owner || !out) return NUX_E_INVALID;
    nux_status status = nux_capability_check(
        owner, receive, NUX_OBJECT_PORT, NUX_RIGHT_RECEIVE, &object_id);
    if (status != NUX_OK) return status;

    nux_arch_lock(&port_lock);
    nux_port_entry *port = nux_port_find(object_id);
    if (!port || port->owner != owner) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
    }
    nux_arch_unlock(&port_lock);

    status = nux_capability_create(owner, NUX_OBJECT_PORT, object_id,
                                   rights | NUX_RIGHT_TRANSFER, out);
    if (status != NUX_OK) return status;

    nux_arch_lock(&port_lock);
    port = nux_port_find(object_id);
    if (!port || port->owner != owner ||
        nux_capability_check(owner, *out, NUX_OBJECT_PORT, rights, &object_id) != NUX_OK) {
        nux_arch_unlock(&port_lock);
        (void)nux_capability_revoke(owner, *out);
        *out = 0;
        return NUX_E_ACCESS;
    }
    nux_arch_unlock(&port_lock);
    return NUX_OK;
}

nux_status nux_port_make_send(nux_task_id owner, nux_capability receive,
                              nux_capability *out)
{
    return nux_port_derive_send(owner, receive, NUX_RIGHT_SEND, out);
}

nux_status nux_port_copy_send(nux_task_id owner, nux_capability send,
                              nux_capability *out)
{
    nux_object_id object_id;
    if (!owner || !out) return NUX_E_INVALID;
    nux_status status = nux_capability_check(
        owner, send, NUX_OBJECT_PORT,
        NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER, &object_id);
    if (status != NUX_OK) return status;
    status = nux_capability_create(owner, NUX_OBJECT_PORT, object_id,
                                   NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER, out);
    if (status != NUX_OK) return status;
    nux_arch_lock(&port_lock);
    nux_port_entry *port = nux_port_find(object_id);
    if (!port || nux_capability_check(owner, *out, NUX_OBJECT_PORT,
                                      NUX_RIGHT_SEND, &object_id) != NUX_OK) {
        nux_arch_unlock(&port_lock);
        (void)nux_capability_revoke(owner, *out);
        *out = 0;
        return NUX_E_NOT_FOUND;
    }
    nux_arch_unlock(&port_lock);
    return NUX_OK;
}

nux_status nux_port_make_send_once(nux_task_id owner, nux_capability receive,
                                   nux_capability *out)
{
    return nux_port_derive_send(owner, receive, NUX_RIGHT_SEND_ONCE, out);
}

nux_status nux_port_destroy(nux_task_id owner, nux_capability control)
{
    nux_object_id object_id;
    nux_status status = nux_capability_check(
        owner, control, NUX_OBJECT_PORT, NUX_RIGHT_CONTROL, &object_id);
    if (status != NUX_OK) return status;

    nux_arch_lock(&port_lock);
    nux_port_entry *port = nux_port_find(object_id);
    nux_object_id verified_object = 0;
    status = nux_capability_check(owner, control, NUX_OBJECT_PORT,
                                  NUX_RIGHT_CONTROL, &verified_object);
    if (!port || status != NUX_OK || verified_object != object_id) {
        nux_arch_unlock(&port_lock);
        return status == NUX_OK ? NUX_E_NOT_FOUND : status;
    }
    if (port->owner != owner) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
    }

    nux_thread_id waiter = port->receiver_waiter;
    port->receiver_waiter = 0;
    while (port->count) {
        nux_u16 encoded = port->queue[port->head];
        if (encoded) messages[encoded - 1].active = 0;
        port->head = (port->head + 1) % port->queue_limit;
        --port->count;
    }
    port->active = 0;
    nux_arch_unlock(&port_lock);
    if (waiter) (void)nux_thread_wake(waiter);

    (void)nux_capability_revoke_object(NUX_OBJECT_PORT, object_id);
    return NUX_OK;
}

static nux_status nux_port_send_internal(nux_task_id sender, nux_capability send,
                                           const nux_message *message,
                                           nux_u64 timeout_ns,
                                           nux_u32 send_once)
{
    nux_object_id object_id;
    nux_status status;
    nux_thread_id waiter = 0;
    nux_rights required = send_once ? NUX_RIGHT_SEND_ONCE : NUX_RIGHT_SEND;
    if (timeout_ns) return NUX_E_NOT_SUPPORTED;
    if (!message || message->inline_size > NUX_MESSAGE_INLINE_MAX ||
        (message->inline_size && !message->inline_data) ||
        message->descriptor_count != 0 ||
        message->kind > (nux_u32)NUX_PORT_MSG_RPC)
        return NUX_E_INVALID;
    status = nux_capability_check(sender, send, NUX_OBJECT_PORT, required, &object_id);
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
    if (send_once) {
        nux_object_id consumed_object = 0;
        status = nux_capability_consume(sender, send, NUX_OBJECT_PORT,
                                        NUX_RIGHT_SEND_ONCE, &consumed_object);
        if (status != NUX_OK || consumed_object != object_id) {
            messages[index].active = 0;
            nux_arch_unlock(&port_lock);
            return status == NUX_OK ? NUX_E_ACCESS : status;
        }
    } else {
        nux_object_id verified_object = 0;
        status = nux_capability_check(sender, send, NUX_OBJECT_PORT,
                                      NUX_RIGHT_SEND, &verified_object);
        if (status != NUX_OK || verified_object != object_id) {
            messages[index].active = 0;
            nux_arch_unlock(&port_lock);
            return status == NUX_OK ? NUX_E_ACCESS : status;
        }
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
    waiter = port->receiver_waiter;
    port->receiver_waiter = 0;
    nux_arch_unlock(&port_lock);
    if (waiter) (void)nux_thread_wake(waiter);
    return NUX_OK;
}

nux_status nux_port_send(nux_task_id sender, nux_capability send, const nux_message *message,
                         nux_u64 timeout_ns)
{
    return nux_port_send_internal(sender, send, message, timeout_ns, 0);
}

nux_status nux_port_send_once(nux_task_id sender, nux_capability send_once,
                              const nux_message *message)
{
    return nux_port_send_internal(sender, send_once, message, 0, 1);
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
    nux_object_id verified_object = 0;
    status = nux_capability_check(receiver, receive, NUX_OBJECT_PORT,
                                  NUX_RIGHT_RECEIVE, &verified_object);
    if (!port || status != NUX_OK || verified_object != object_id) {
        nux_arch_unlock(&port_lock);
        return status == NUX_OK ? NUX_E_NOT_FOUND : status;
    }
    if (port->owner != receiver) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
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

nux_status nux_port_receive_wait(nux_task_id receiver,
                                  nux_capability receive,
                                  nux_message *message,
                                  nux_u64 timeout_ns)
{
    if (!message) return NUX_E_INVALID;
    if (timeout_ns) return NUX_E_NOT_SUPPORTED;

    for (;;) {
        nux_status status = nux_port_receive(receiver, receive, message, 0);
        if (status != NUX_E_WOULD_BLOCK) return status;

        nux_object_id object_id = 0;
        status = nux_capability_check(receiver, receive, NUX_OBJECT_PORT,
                                      NUX_RIGHT_RECEIVE, &object_id);
        if (status != NUX_OK) return status;

        nux_arch_lock(&port_lock);
        nux_port_entry *port = nux_port_find(object_id);
        nux_object_id verified_object = 0;
        status = nux_capability_check(receiver, receive, NUX_OBJECT_PORT,
                                      NUX_RIGHT_RECEIVE, &verified_object);
        if (!port || status != NUX_OK || verified_object != object_id) {
            nux_arch_unlock(&port_lock);
            return status == NUX_OK ? NUX_E_NOT_FOUND : status;
        }
        if (port->owner != receiver) {
            nux_arch_unlock(&port_lock);
            return NUX_E_ACCESS;
        }
        if (port->count) {
            nux_arch_unlock(&port_lock);
            continue;
        }
        if (port->receiver_waiter) {
            nux_arch_unlock(&port_lock);
            return NUX_E_BUSY;
        }

        nux_thread_id waiter = 0;
        status = nux_thread_prepare_wait_current(&waiter);
        if (status != NUX_OK) {
            nux_arch_unlock(&port_lock);
            return status;
        }
        port->receiver_waiter = waiter;
        nux_arch_unlock(&port_lock);

        status = nux_thread_yield();
        if (status != NUX_OK) {
            nux_arch_lock(&port_lock);
            port = nux_port_find(object_id);
            if (port && port->receiver_waiter == waiter)
                port->receiver_waiter = 0;
            nux_arch_unlock(&port_lock);
            (void)nux_thread_wake(waiter);
            return status;
        }
    }
}

nux_status nux_port_move_right(nux_task_id source_owner,
                               nux_capability source,
                               nux_task_id destination,
                               nux_port_right_kind right,
                               nux_capability *out)
{
    nux_task_info task;
    nux_rights required;
    nux_rights granted;
    nux_object_id object_id = 0;
    nux_thread_id waiter = 0;

    if (!source_owner || !destination || source_owner == destination ||
        !out || right < NUX_PORT_RIGHT_SEND ||
        right > NUX_PORT_RIGHT_SEND_ONCE)
        return NUX_E_INVALID;
    *out = 0;
    if (nux_task_get(source_owner, &task) != NUX_OK ||
        task.state == NUX_TASK_DYING || task.state == NUX_TASK_UNUSED)
        return NUX_E_ACCESS;
    if (nux_task_get(destination, &task) != NUX_OK ||
        task.state == NUX_TASK_DYING || task.state == NUX_TASK_UNUSED)
        return NUX_E_ACCESS;

    if (right == NUX_PORT_RIGHT_SEND) {
        required = NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER;
        granted = NUX_RIGHT_SEND | NUX_RIGHT_TRANSFER;
    } else if (right == NUX_PORT_RIGHT_RECEIVE) {
        required = NUX_RIGHT_RECEIVE | NUX_RIGHT_CONTROL | NUX_RIGHT_TRANSFER;
        granted = NUX_RIGHT_RECEIVE | NUX_RIGHT_READ |
            NUX_RIGHT_CONTROL | NUX_RIGHT_TRANSFER;
    } else {
        required = NUX_RIGHT_SEND_ONCE | NUX_RIGHT_TRANSFER;
        granted = NUX_RIGHT_SEND_ONCE | NUX_RIGHT_TRANSFER;
    }

    nux_arch_lock(&port_lock);
    if (nux_task_get(source_owner, &task) != NUX_OK ||
        task.state == NUX_TASK_DYING || task.state == NUX_TASK_UNUSED ||
        nux_task_get(destination, &task) != NUX_OK ||
        task.state == NUX_TASK_DYING || task.state == NUX_TASK_UNUSED) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
    }

    nux_status status = nux_capability_check(
        source_owner, source, NUX_OBJECT_PORT, required, &object_id);
    if (status != NUX_OK) {
        nux_arch_unlock(&port_lock);
        return status;
    }
    nux_port_entry *port = nux_port_find(object_id);
    if (!port || (right == NUX_PORT_RIGHT_RECEIVE &&
                  port->owner != source_owner)) {
        nux_arch_unlock(&port_lock);
        return NUX_E_NOT_FOUND;
    }

    status = nux_capability_move(source_owner, source, destination,
                                 NUX_OBJECT_PORT, required, granted,
                                 &object_id, out);
    if (status != NUX_OK) {
        nux_arch_unlock(&port_lock);
        return status;
    }

    if (right == NUX_PORT_RIGHT_RECEIVE) {
        for (nux_u32 i = 0; i < NUX_PORT_SET_LIMIT; ++i) {
            nux_port_set_entry *set = &port_sets[i];
            if (!set->active || set->owner != source_owner) continue;
            for (nux_u32 j = 0; j < set->count;) {
                if (set->ports[j] != source) { ++j; continue; }
                for (nux_u32 k = j + 1; k < set->count; ++k)
                    set->ports[k - 1] = set->ports[k];
                --set->count;
            }
        }
        port->owner = destination;
        waiter = port->receiver_waiter;
        port->receiver_waiter = 0;
        (void)nux_capability_revoke_owner_object_rights(
            source_owner, NUX_OBJECT_PORT, object_id,
            NUX_RIGHT_RECEIVE | NUX_RIGHT_CONTROL);
    }
    nux_arch_unlock(&port_lock);
    if (waiter) (void)nux_thread_wake(waiter);
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
        set->receiver_waiter = 0;
        set->id = next_port_set_id++;
        if (!set->id) set->id = next_port_set_id++;
        *out = set->id;
        nux_port_set_id created_id = set->id;
        nux_arch_unlock(&port_lock);
        if (nux_task_get(owner, &task) != NUX_OK ||
            task.state == NUX_TASK_UNUSED || task.state == NUX_TASK_DYING) {
            nux_arch_lock(&port_lock);
            set = nux_port_set_find(created_id);
            if (set && set->owner == owner) {
                set->active = 0;
                set->count = 0;
            }
            nux_arch_unlock(&port_lock);
            *out = 0;
            return NUX_E_ACCESS;
        }
        return NUX_OK;
    }
    nux_arch_unlock(&port_lock);
    return NUX_E_LIMIT;
}

nux_status nux_port_set_add(nux_task_id owner, nux_port_set_id id,
                             nux_capability receive)
{
    nux_object_id object_id;
    nux_thread_id waiter = 0;
    nux_status status;
    if (!owner) return NUX_E_INVALID;

    status = nux_capability_check(owner, receive, NUX_OBJECT_PORT,
                                  NUX_RIGHT_RECEIVE, &object_id);
    if (status != NUX_OK) return status;

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
        if (set->ports[i] == receive) {
            nux_arch_unlock(&port_lock);
            return NUX_OK;
        }
    }
    nux_port_entry *port = nux_port_find(object_id);
    if (!port || port->owner != owner) {
        nux_arch_unlock(&port_lock);
        return NUX_E_ACCESS;
    }
    if (set->count == NUX_PORT_SET_WIDTH) {
        nux_arch_unlock(&port_lock);
        return NUX_E_LIMIT;
    }
    set->ports[set->count] = receive;
    set->object_ids[set->count] = object_id;
    ++set->count;
    waiter = set->receiver_waiter;
    set->receiver_waiter = 0;
    nux_arch_unlock(&port_lock);
    if (waiter) (void)nux_thread_wake(waiter);
    return NUX_OK;
}

nux_status nux_port_set_remove(nux_task_id owner, nux_port_set_id id,
                                nux_capability receive)
{
    nux_thread_id waiter = 0;
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
        for (nux_u32 j = i + 1; j < set->count; ++j) {
            set->ports[j - 1] = set->ports[j];
            set->object_ids[j - 1] = set->object_ids[j];
        }
        --set->count;
        waiter = set->receiver_waiter;
        set->receiver_waiter = 0;
        nux_arch_unlock(&port_lock);
        if (waiter) (void)nux_thread_wake(waiter);
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
