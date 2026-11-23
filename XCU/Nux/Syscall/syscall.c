#include <nux/syscall.h>
#include <nux/thread.h>
#include <nux/task.h>
#include <nux/vm.h>
#include <nux/port.h>
#include <nux/capability.h>

extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);

volatile nux_u64 nux_last_user_exit_code;
static nux_syscall_entry calls[NUX_SYSCALL_LIMIT];
static volatile nux_u32 syscall_lock;


static nux_status syscall_exit_handler(nux_syscall_word a, nux_syscall_word b,
                                        nux_syscall_word c, nux_syscall_word d,
                                        nux_syscall_word e, nux_syscall_word f,
                                        nux_syscall_word *result)
{
    nux_last_user_exit_code = a;
    (void)b; (void)c; (void)d; (void)e; (void)f;
    *result = 0;
    return nux_thread_exit_current();
}

static nux_status syscall_yield_handler(nux_syscall_word a, nux_syscall_word b,
                                        nux_syscall_word c, nux_syscall_word d,
                                        nux_syscall_word e, nux_syscall_word f,
                                        nux_syscall_word *result)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    *result = 0;
    return nux_thread_yield();
}


#include <nux/task.h>
#include <nux/vm.h>
#include <nux/port.h>

static nux_status syscall_user_context(nux_task_id *task, nux_u64 *space)
{
    if (!task || !space) return NUX_E_INVALID;
    *task = nux_thread_current_task();
    if (!*task) return NUX_E_BAD_STATE;
    return nux_task_get_address_space(*task, space);
}

static nux_status syscall_copy_in(nux_u64 space, nux_u64 address,
                                  void *destination, nux_size size)
{
    if (!address || !destination || !size) return NUX_E_INVALID;
    return nux_vm_read(space, address, destination, size);
}

static nux_status syscall_copy_out(nux_u64 space, nux_u64 address,
                                   const void *source, nux_size size)
{
    if (!address || !source || !size) return NUX_E_INVALID;
    return nux_vm_write(space, address, source, size);
}

static nux_status syscall_port_create(nux_syscall_word send_out,
                                      nux_syscall_word receive_out,
                                      nux_syscall_word c, nux_syscall_word d,
                                      nux_syscall_word e, nux_syscall_word f,
                                      nux_syscall_word *result)
{
    nux_task_id task; nux_u64 space; nux_capability send, receive;
    (void)c; (void)d; (void)e; (void)f; *result = 0;
    nux_status status = syscall_user_context(&task, &space);
    if (status != NUX_OK) return status;
    status = nux_port_create(task, NUX_PORT_QUEUE_DEFAULT, &send, &receive);
    if (status != NUX_OK) return status;
    status = syscall_copy_out(space, send_out, &send, sizeof(send));
    if (status == NUX_OK)
        status = syscall_copy_out(space, receive_out, &receive, sizeof(receive));
    if (status != NUX_OK) (void)nux_port_destroy(task, receive);
    return status;
}

static nux_status syscall_port_send(nux_syscall_word handle,
                                    nux_syscall_word message_address,
                                    nux_syscall_word c, nux_syscall_word d,
                                    nux_syscall_word e, nux_syscall_word f,
                                    nux_syscall_word *result)
{
    nux_task_id task; nux_u64 space; nux_user_message user;
    nux_message message; nux_u8 payload[NUX_MESSAGE_INLINE_MAX];
    (void)c; (void)d; (void)e; (void)f; *result = 0;
    if (handle > 0xffffffffULL) return NUX_E_INVALID;
    nux_status status = syscall_user_context(&task, &space);
    if (status != NUX_OK) return status;
    status = syscall_copy_in(space, message_address, &user, sizeof(user));
    if (status != NUX_OK) return status;
    if (user.inline_size > NUX_MESSAGE_INLINE_MAX ||
        user.descriptor_count || (user.inline_size && !user.inline_data))
        return NUX_E_INVALID;
    if (user.inline_size) {
        status = syscall_copy_in(space, user.inline_data, payload, user.inline_size);
        if (status != NUX_OK) return status;
    }
    message.id = user.id;
    message.kind = user.kind;
    message.sequence = 0;
    message.deadline_ns = user.deadline_ns;
    message.inline_size = user.inline_size;
    message.descriptor_count = 0;
    message.inline_data = payload;
    message.inline_capacity = 0;
    return nux_port_send(task, (nux_capability)handle, &message, 0);
}

static nux_status syscall_port_receive(nux_syscall_word handle,
                                       nux_syscall_word message_address,
                                       nux_syscall_word c, nux_syscall_word d,
                                       nux_syscall_word e, nux_syscall_word f,
                                       nux_syscall_word *result)
{
    nux_task_id task; nux_u64 space; nux_user_message user;
    nux_message message; nux_u8 payload[NUX_MESSAGE_INLINE_MAX];
    (void)c; (void)d; (void)e; (void)f; *result = 0;
    if (handle > 0xffffffffULL) return NUX_E_INVALID;
    nux_status status = syscall_user_context(&task, &space);
    if (status != NUX_OK) return status;
    status = syscall_copy_in(space, message_address, &user, sizeof(user));
    if (status != NUX_OK) return status;
    if (user.inline_capacity > NUX_MESSAGE_INLINE_MAX ||
        (user.inline_capacity && !user.inline_data))
        return NUX_E_INVALID;
    message.id = user.id;
    message.kind = user.kind;
    message.sequence = user.sequence;
    message.deadline_ns = user.deadline_ns;
    message.inline_size = 0;
    message.descriptor_count = 0;
    message.inline_data = payload;
    message.inline_capacity = user.inline_capacity;
    status = nux_port_receive(task, (nux_capability)handle, &message, 0);
    if (status != NUX_OK) return status;
    if (message.inline_size > user.inline_capacity) return NUX_E_BAD_STATE;
    if (message.inline_size) {
        status = syscall_copy_out(space, user.inline_data, payload, message.inline_size);
        if (status != NUX_OK) return status;
    }
    user.id = message.id;
    user.kind = message.kind;
    user.sequence = message.sequence;
    user.deadline_ns = message.deadline_ns;
    user.inline_size = message.inline_size;
    user.descriptor_count = message.descriptor_count;
    return syscall_copy_out(space, message_address, &user, sizeof(user));
}

static nux_status syscall_port_destroy(nux_syscall_word handle,
                                        nux_syscall_word b, nux_syscall_word c,
                                        nux_syscall_word d, nux_syscall_word e,
                                        nux_syscall_word f, nux_syscall_word *result)
{
    nux_task_id task; nux_u64 space;
    (void)b; (void)c; (void)d; (void)e; (void)f; *result = 0;
    if (handle > 0xffffffffULL) return NUX_E_INVALID;
    nux_status status = syscall_user_context(&task, &space);
    if (status != NUX_OK) return status;
    return nux_port_destroy(task, (nux_capability)handle);
}

static nux_status syscall_port_make_send(nux_syscall_word receive_handle,
                                         nux_syscall_word output,
                                         nux_syscall_word c, nux_syscall_word d,
                                         nux_syscall_word e, nux_syscall_word f,
                                         nux_syscall_word *result)
{
    nux_task_id task; nux_u64 space; nux_capability send;
    (void)c; (void)d; (void)e; (void)f; *result = 0;
    if (receive_handle > 0xffffffffULL) return NUX_E_INVALID;
    nux_status status = syscall_user_context(&task, &space);
    if (status != NUX_OK) return status;
    status = nux_port_make_send(task, (nux_capability)receive_handle, &send);
    if (status != NUX_OK) return status;
    status = syscall_copy_out(space, output, &send, sizeof(send));
    if (status != NUX_OK) (void)nux_capability_revoke(task, send);
    return status;
}

static nux_status syscall_port_make_send_once(nux_syscall_word receive_handle,
                                              nux_syscall_word output,
                                              nux_syscall_word c, nux_syscall_word d,
                                              nux_syscall_word e, nux_syscall_word f,
                                              nux_syscall_word *result)
{
    nux_task_id task; nux_u64 space; nux_capability send_once;
    (void)c; (void)d; (void)e; (void)f; *result = 0;
    if (receive_handle > 0xffffffffULL) return NUX_E_INVALID;
    nux_status status = syscall_user_context(&task, &space);
    if (status != NUX_OK) return status;
    status = nux_port_make_send_once(task, (nux_capability)receive_handle, &send_once);
    if (status != NUX_OK) return status;
    status = syscall_copy_out(space, output, &send_once, sizeof(send_once));
    if (status != NUX_OK) (void)nux_capability_revoke(task, send_once);
    return status;
}


static nux_status syscall_port_send_once(nux_syscall_word handle,
                                         nux_syscall_word message_address,
                                         nux_syscall_word c, nux_syscall_word d,
                                         nux_syscall_word e, nux_syscall_word f,
                                         nux_syscall_word *result)
{
    nux_task_id task; nux_u64 space; nux_user_message user;
    nux_message message; nux_u8 payload[NUX_MESSAGE_INLINE_MAX];
    (void)c; (void)d; (void)e; (void)f; *result = 0;
    if (handle > 0xffffffffULL) return NUX_E_INVALID;
    nux_status status = syscall_user_context(&task, &space);
    if (status != NUX_OK) return status;
    status = syscall_copy_in(space, message_address, &user, sizeof(user));
    if (status != NUX_OK) return status;
    if (user.inline_size > NUX_MESSAGE_INLINE_MAX ||
        user.descriptor_count || (user.inline_size && !user.inline_data))
        return NUX_E_INVALID;
    if (user.inline_size) {
        status = syscall_copy_in(space, user.inline_data, payload, user.inline_size);
        if (status != NUX_OK) return status;
    }
    message.id = user.id;
    message.kind = user.kind;
    message.sequence = 0;
    message.deadline_ns = user.deadline_ns;
    message.inline_size = user.inline_size;
    message.descriptor_count = 0;
    message.inline_data = payload;
    message.inline_capacity = 0;
    return nux_port_send_once(task, (nux_capability)handle, &message);
}

nux_status nux_syscall_system_init(void)
{
    nux_arch_lock(&syscall_lock);
    for (nux_u32 i = 0; i < NUX_SYSCALL_LIMIT; ++i) {
        calls[i].number = i;
        calls[i].flags = 0;
        calls[i].handler = NUX_NULL;
    }
    nux_arch_unlock(&syscall_lock);
    nux_last_user_exit_code = 0;
    nux_status status = nux_syscall_register(NUX_SYSCALL_EXIT, syscall_exit_handler, 0);
    if (status != NUX_OK) return status;
    status = nux_syscall_register(NUX_SYSCALL_YIELD, syscall_yield_handler, 0);
    if (status != NUX_OK) return status;
    status = nux_syscall_register(NUX_SYSCALL_PORT_CREATE, syscall_port_create, 0);
    if (status != NUX_OK) return status;
    status = nux_syscall_register(NUX_SYSCALL_PORT_SEND, syscall_port_send, 0);
    if (status != NUX_OK) return status;
    status = nux_syscall_register(NUX_SYSCALL_PORT_RECEIVE, syscall_port_receive, 0);
    if (status != NUX_OK) return status;
    status = nux_syscall_register(NUX_SYSCALL_PORT_DESTROY, syscall_port_destroy, 0);
    if (status != NUX_OK) return status;
    status = nux_syscall_register(NUX_SYSCALL_PORT_MAKE_SEND, syscall_port_make_send, 0);
    if (status != NUX_OK) return status;
    status = nux_syscall_register(NUX_SYSCALL_PORT_MAKE_SEND_ONCE, syscall_port_make_send_once, 0);
    if (status != NUX_OK) return status;
    return nux_syscall_register(NUX_SYSCALL_PORT_SEND_ONCE, syscall_port_send_once, 0);
}

nux_status nux_syscall_register(nux_u32 number, nux_syscall_handler handler,
                                nux_u32 flags)
{
    if (number >= NUX_SYSCALL_LIMIT || !handler)
        return NUX_E_INVALID;
    nux_arch_lock(&syscall_lock);
    if (calls[number].handler) {
        nux_arch_unlock(&syscall_lock);
        return NUX_E_BUSY;
    }
    calls[number].flags = flags;
    calls[number].handler = handler;
    nux_arch_unlock(&syscall_lock);
    return NUX_OK;
}

nux_status nux_syscall_dispatch(nux_u64 number,
                                const nux_syscall_word arguments[6],
                                nux_syscall_word *result)
{
    nux_syscall_handler handler;
    if (number >= NUX_SYSCALL_LIMIT || !arguments || !result)
        return NUX_E_INVALID;
    nux_arch_lock(&syscall_lock);
    handler = calls[number].handler;
    nux_arch_unlock(&syscall_lock);
    if (!handler) return NUX_E_NOT_SUPPORTED;
    *result = 0;
    return handler(arguments[0], arguments[1], arguments[2],
                   arguments[3], arguments[4], arguments[5], result);
}

nux_syscall_word nux_syscall_invoke(nux_u64 number,
                                    const nux_syscall_word arguments[6])
{
    nux_syscall_word result = 0;
    if (number >= NUX_SYSCALL_LIMIT)
        return (nux_syscall_word)(nux_i64)NUX_E_INVALID;
    nux_status status = nux_syscall_dispatch(number, arguments, &result);
    return status == NUX_OK ? result : (nux_syscall_word)(nux_i64)status;
}
