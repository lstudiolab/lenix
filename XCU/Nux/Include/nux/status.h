#ifndef NUX_STATUS_H
#define NUX_STATUS_H

#include <nux/types.h>

typedef nux_i32 nux_status;

enum {
    NUX_OK                 =  0,
    NUX_E_INVALID          = -1,
    NUX_E_NOT_FOUND        = -2,
    NUX_E_ACCESS           = -3,
    NUX_E_NO_MEMORY        = -4,
    NUX_E_BUSY             = -5,
    NUX_E_TIMEOUT          = -6,
    NUX_E_INTERRUPTED      = -7,
    NUX_E_BAD_STATE        = -8,
    NUX_E_NOT_SUPPORTED    = -9,
    NUX_E_WOULD_BLOCK      = -10,
    NUX_E_LIMIT            = -11
};

#endif
