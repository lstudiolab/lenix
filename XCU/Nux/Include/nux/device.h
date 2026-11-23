#ifndef NUX_DEVICE_H
#define NUX_DEVICE_H
#include <nux/types.h>
#include <nux/status.h>
#include <nux/capability.h>
#define NUX_DEVICE_NAME_MAX 64u
typedef nux_u64 nux_device_id;
typedef struct { nux_device_id id; char name[NUX_DEVICE_NAME_MAX]; nux_u32 class_id; nux_u32 flags; nux_u64 base; nux_u64 length; } nux_device_info;
nux_status nux_device_system_init(void);
nux_status nux_device_register(const nux_device_info *info, nux_capability owner);
nux_status nux_device_open(nux_device_id device, nux_capability *handle);
nux_status nux_device_close(nux_capability handle);
nux_status nux_device_read(nux_capability handle, nux_u64 offset, void *buffer, nux_size length, nux_size *transferred);
nux_status nux_device_write(nux_capability handle, nux_u64 offset, const void *buffer, nux_size length, nux_size *transferred);
nux_status nux_device_map(nux_capability handle, nux_u64 offset, nux_u64 length, nux_u64 *virtual_address);
nux_status nux_device_control(nux_capability handle, nux_u32 request, void *data, nux_size length);
#endif
