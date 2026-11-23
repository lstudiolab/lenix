#include <nux/types.h>

void *memcpy(void *destination, const void *source, nux_size size)
{
    nux_u8 *out = (nux_u8 *)destination;
    const nux_u8 *in = (const nux_u8 *)source;
    for (nux_size i = 0; i < size; ++i) out[i] = in[i];
    return destination;
}

void *memmove(void *destination, const void *source, nux_size size)
{
    nux_u8 *out = (nux_u8 *)destination;
    const nux_u8 *in = (const nux_u8 *)source;
    if (out == in || size == 0) return destination;
    if ((nux_uptr)out < (nux_uptr)in) {
        for (nux_size i = 0; i < size; ++i) out[i] = in[i];
    } else {
        while (size != 0) {
            --size;
            out[size] = in[size];
        }
    }
    return destination;
}

void *memset(void *destination, int value, nux_size size)
{
    nux_u8 *out = (nux_u8 *)destination;
    for (nux_size i = 0; i < size; ++i) out[i] = (nux_u8)value;
    return destination;
}

int memcmp(const void *left, const void *right, nux_size size)
{
    const nux_u8 *a = (const nux_u8 *)left;
    const nux_u8 *b = (const nux_u8 *)right;
    for (nux_size i = 0; i < size; ++i) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}
