/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_SYSTEM_H_
#define _MESHX_SYSTEM_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

/**
 * @brief system abort
 * @note when invoke this function, system shall be aborted
 */
MESHX_EXTERN void meshx_abort(void);

MESHX_EXTERN void meshx_shutdown(void);
MESHX_EXTERN void meshx_reboot(uint32_t reason);

MESHX_END_DECLS


#endif /* _MESHX_SYSTEM_H_ */