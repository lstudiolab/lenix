#include <nux/syscall.h>
#include <nux/task.h>
#include <nux/vm.h>
#include <nux/port.h>

void nux_arch_lock(volatile nux_u32 *lock)
{
    while (*lock) { }
    *lock = 1;
}
void nux_arch_unlock(volatile nux_u32 *lock) { *lock = 0; }
nux_status nux_thread_yield(void) { return NUX_OK; }
nux_status nux_thread_exit_current(void) { return NUX_OK; }
nux_task_id nux_thread_current_task(void) { return 0; }
nux_status nux_task_get_address_space(nux_task_id task,nux_u64 *space) {(void)task; if(space)*space=1; return NUX_OK;}
nux_status nux_vm_read(nux_u64 space,nux_u64 address,void *destination,nux_size size) {(void)space;(void)address;(void)destination;(void)size;return NUX_E_ACCESS;}
nux_status nux_vm_write(nux_u64 space,nux_u64 address,const void *source,nux_size size) {(void)space;(void)address;(void)source;(void)size;return NUX_E_ACCESS;}
nux_status nux_port_create(nux_task_id owner,nux_u32 limit,nux_capability *send,nux_capability *receive) {(void)owner;(void)limit;(void)send;(void)receive;return NUX_E_NOT_SUPPORTED;}
nux_status nux_port_send(nux_task_id sender,nux_capability send,const nux_message *message,nux_u64 timeout) {(void)sender;(void)send;(void)message;(void)timeout;return NUX_E_NOT_SUPPORTED;}
nux_status nux_port_receive(nux_task_id receiver,nux_capability receive,nux_message *message,nux_u64 timeout) {(void)receiver;(void)receive;(void)message;(void)timeout;return NUX_E_NOT_SUPPORTED;}
nux_status nux_port_destroy(nux_task_id owner,nux_capability control) {(void)owner;(void)control;return NUX_E_NOT_SUPPORTED;}
nux_status nux_port_make_send(nux_task_id owner,nux_capability receive,nux_capability *out) {(void)owner;(void)receive;(void)out;return NUX_E_NOT_SUPPORTED;}
nux_status nux_port_make_send_once(nux_task_id owner,nux_capability receive,nux_capability *out) {(void)owner;(void)receive;(void)out;return NUX_E_NOT_SUPPORTED;}

static nux_status add_handler(nux_syscall_word a, nux_syscall_word b,
                              nux_syscall_word c, nux_syscall_word d,
                              nux_syscall_word e, nux_syscall_word f,
                              nux_syscall_word *result)
{
    *result = a + b + c + d + e + f;
    return NUX_OK;
}

int main(void)
{
    nux_syscall_word args[6] = {1, 2, 3, 4, 5, 6};
    nux_syscall_word result = 99;
    if (nux_syscall_system_init() != NUX_OK) return 1;
    if (nux_syscall_dispatch(8, args, &result) != NUX_E_NOT_SUPPORTED) return 2;
    if (nux_syscall_dispatch(0x100000000ULL, args, &result) != NUX_E_INVALID) return 11;
    if (nux_syscall_dispatch(NUX_SYSCALL_YIELD, args, &result) != NUX_OK) return 8;
    if (nux_syscall_invoke(NUX_SYSCALL_YIELD, args) != 0) return 9;
    if ((nux_i64)nux_syscall_invoke(999, args) != NUX_E_INVALID) return 10;
    if (nux_syscall_register(8, add_handler, 0) != NUX_OK) return 3;
    if (nux_syscall_register(7, add_handler, 0) != NUX_E_BUSY) return 4;
    if (nux_syscall_dispatch(8, args, &result) != NUX_OK) return 5;
    if (result != 21) return 6;
    if (nux_syscall_register(NUX_SYSCALL_LIMIT, add_handler, 0) != NUX_E_INVALID) return 7;
    return 0;
}
