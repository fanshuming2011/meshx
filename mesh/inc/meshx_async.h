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


typedef struct _meshx_async_msg meshx_async_msg_t;

typedef int32_t (*meshx_async_msg_notify_t)(const meshx_async_msg_t *pmsg);

MESHX_EXTERN int32_t meshx_async_msg_set_notify(meshx_async_msg_notify_t handler);
MESHX_EXTERN void meshx_async_msg_process(meshx_async_msg_t *pmsg);


MESHX_END_DECLS

#endif /* _MESHX_ASYNC_H_ */
