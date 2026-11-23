#include <nux/types.h>
#include <nux/boot.h>
#include <nux/status.h>
#include <nux/object.h>
#include <nux/memory.h>
#include <nux/task.h>
#include <nux/thread.h>
#include <nux/capability.h>
#include <nux/port.h>
#include <nux/vm.h>
#include <nux/sync.h>
#include <nux/processor.h>
#include <nux/exception.h>
#include <nux/time.h>
#include <nux/bootstrap.h>
#include <nux/device.h>
#include <nux/host.h>
#include <nux/debug.h>
#include <nux/syscall.h>

nux_status nux_header_contract_check(void)
{
    nux_message message = {0};
    nux_thread_info thread = {0};
    nux_vm_region region = {0};
    nux_processor_info processor = {0};
    nux_host_info host = {0};
    return (nux_status)(message.id + thread.id + region.start +
                        processor.id + host.cpu_count);
}
