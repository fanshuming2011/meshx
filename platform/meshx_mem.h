/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_MEM_H_
#define _MESHX_MEM_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN void *meshx_malloc(size_t size);
MESHX_EXTERN void meshx_free(void *ptr);

MESHX_END_DECLS


#endif