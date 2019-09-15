/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_LIB_H_
#define _MESHX_LIB_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_atoi(const char *pstr, bool *pvalid);
MESHX_EXTERN void meshx_itoa(int32_t val, char *pstr);
MESHX_EXTERN void meshx_bin2hex(const char *pbin, uint8_t *phex, uint8_t len);
MESHX_EXTERN void meshx_swap(uint8_t *x, uint8_t *y);

MESHX_END_DECLS

#endif

