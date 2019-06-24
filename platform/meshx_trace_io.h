/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 *
 */
#ifndef _MESHX_TRACE_IO_H_
#define _MESHX_TRACE_IO_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_trace_io_init(void);
MESHX_EXTERN int32_t meshx_trace_send(const char *pdata, uint32_t len);

MESHX_END_DECLS

#endif