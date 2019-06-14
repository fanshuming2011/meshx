/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_ASYNC_H_
#define _MESHX_ASYNC_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS


typedef int32_t (*meshx_async_msg_notify_t)(void);

MESHX_EXTERN int32_t meshx_async_msg_init(uint32_t msg_max_num, meshx_async_msg_notify_t handler);
MESHX_EXTERN void meshx_async_msg_process(void);


MESHX_END_DECLS

#endif /* _MESHX_ASYNC_H_ */
