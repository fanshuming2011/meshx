/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_ASSERT_H_
#define _MESHX_ASSERT_H_

#include "meshx_types.h"
#include "meshx_config.h"

MESHX_BEGIN_DECLS


#if MESHX_ENABLE_ASSERT
MESHX_EXTERN void meshx_assert(bool condition,
                               const char  *condition_text,
                               const char  *file,
                               int          line,
                               const char  *func);
#define MESHX_ASSERT(condition) \
    meshx_assert((condition) != 0, #condition, __FILE__, __LINE__, __FUNC__)
#else
#define MESHX_ASSERT(condition) do { } while (0)
#endif


MESHX_END_DECLS

#endif /* _MESHX_ASSERT_H_ */