/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_MSG_QUEUE_H_
#define _MESHX_MSG_QUEUE_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef void *meshx_msg_queue_t;

MESHX_EXTERN int32_t meshx_msg_queue_receive(meshx_msg_queue_t msg_queue, void *pmsg,
                                             uint32_t wait_ms);
MESHX_EXTERN int32_t meshx_msg_queue_send(meshx_msg_queue_t msg_queue, void *pmsg,
                                          uint32_t wait_ms);

MESHX_END_DECLS

#endif /* _MESHX_MSG_QUEUE_H_ */