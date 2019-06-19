/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_MISC_H_
#define _MESHX_MISC_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN void meshx_srand(uint32_t seed);
MESHX_EXTERN int32_t meshx_rand(void);
MESHX_EXTERN int32_t meshx_trace_send(const char *pdata, uint32_t len);

MESHX_END_DECLS

#endif /* _MESHX_MISC_H_ */

