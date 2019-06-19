/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_IO_H_
#define _MESHX_IO_H_

#include <stdarg.h>
#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef int32_t (*io_write)(const char *pdata, uint32_t len);

MESHX_EXTERN int32_t meshx_printf(io_write pwrite, const char *fmt, ...);
MESHX_EXTERN int32_t meshx_vsprintf(io_write pwrite, const char *fmt, va_list args);

MESHX_END_DECLS

#endif
