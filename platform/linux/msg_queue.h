/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

#include "meshx_types.h"


MESHX_BEGIN_DECLS

typedef void *meshx_msg_queue_t;

MESHX_EXTERN void msg_queue_init(void);

MESHX_EXTERN int32_t msg_queue_create(meshx_msg_queue_t *pmsg_queue, uint32_t msg_num,
                                      uint32_t msg_size);
MESHX_EXTERN void msg_queue_delete(meshx_msg_queue_t msg_queue);
MESHX_EXTERN int32_t msg_queue_receive(meshx_msg_queue_t msg_queue, void *pmsg,
                                       uint32_t wait_ms);
MESHX_EXTERN int32_t msg_queue_send(meshx_msg_queue_t msg_queue, void *pmsg, uint32_t wait_ms);

MESHX_END_DECLS

#endif /* _MSG_QUEUE_H_ */