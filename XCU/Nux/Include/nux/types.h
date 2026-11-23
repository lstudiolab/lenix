#ifndef NUX_TYPES_H
#define NUX_TYPES_H

/*
 * Nux base types.
 * Keep these definitions freestanding and independent of hosted libc.
 */
typedef __UINT8_TYPE__  nux_u8;
typedef __UINT16_TYPE__ nux_u16;
typedef __UINT32_TYPE__ nux_u32;
typedef __UINT64_TYPE__ nux_u64;
typedef __INT8_TYPE__   nux_i8;
typedef __INT16_TYPE__  nux_i16;
typedef __INT32_TYPE__  nux_i32;
typedef __INT64_TYPE__  nux_i64;
typedef __SIZE_TYPE__   nux_size;
typedef __UINTPTR_TYPE__ nux_uptr;
typedef __INTPTR_TYPE__  nux_iptr;

#define NUX_NULL ((void *)0)
#define NUX_U64_MAX ((nux_u64)~(nux_u64)0)

#endif
