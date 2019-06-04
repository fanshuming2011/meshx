/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include "meshx_msg_queue.h"
#include "meshx_errno.h"

int32_t meshx_msg_queue_create(meshx_msg_queue_t *pmsg_queue, uint32_t msg_num, uint32_t msg_size)
{
    return MESHX_SUCCESS;
}

void meshx_msg_queue_delete(meshx_msg_queue_t msg_queue)
{

}

int32_t meshx_msg_queue_receive(meshx_msg_queue_t msg_queue, void *pmsg, uint32_t wait_ms)
{
    return MESHX_SUCCESS;
}

int32_t meshx_msg_queue_peek(meshx_msg_queue_t msg_queue, void *pmsg, uint32_t wait_ms)
{
    return MESHX_SUCCESS;
}

int32_t meshx_msg_queue_send(meshx_msg_queue_t msg_queue, void *pmsg)
{
    return MESHX_SUCCESS;
}

