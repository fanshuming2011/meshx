/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_TYPES_H_
#define _MESHX_TYPES_H_

#include "meshx_macros.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

MESHX_BEGIN_DECLS

/* boolean definition */
#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif


#ifndef NULL
#ifdef __cplusplus
#define NULL    (0L)
#else
#define NULL    ((void*) 0)
#endif
#endif

MESHX_END_DECLS

#endif /* _MESHX_TYPES_H_ */