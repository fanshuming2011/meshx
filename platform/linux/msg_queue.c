
/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <unistd.h>
#define TRACE_MOULE "MSG_QUEUE"
#include "meshx_trace.h"
#include "meshx_msg_queue.h"
#include "meshx_errno.h"
#include "meshx_mem.h"

typedef struct
{
    int fd_read;
    int fd_write;
    uint32_t msg_num;
    uint32_t msg_size;
} meshx_msg_queue_wrapper_t;


int32_t meshx_msg_queue_create(meshx_msg_queue_t *pmsg_queue, uint32_t msg_num, uint32_t msg_size)
{
    meshx_msg_queue_wrapper_t *pmsg = meshx_malloc(sizeof(meshx_msg_queue_wrapper_t));
    if (NULL == pmsg)
    {
        MESHX_ERROR("create message queue failed: out of memory");
        return MESHX_ERR_MEM;
    }

    int fd[2];
    if (-1 == pipe(fd))
    {
        MESHX_ERROR("create message queue failed: no pipe resource");
        meshx_free(pmsg);
        return MESHX_ERR_RESOURCE;
    }

    *pmsg_queue = pmsg;
    pmsg->fd_read = fd[0];
    pmsg->fd_write = fd[1];
    pmsg->msg_num = msg_num;
    pmsg->msg_size = msg_size;

    return MESHX_SUCCESS;
}

void meshx_msg_queue_delete(meshx_msg_queue_t msg_queue)
{
    meshx_msg_queue_wrapper_t *pmsg = msg_queue;
    close(pmsg->fd_read);
    close(pmsg->fd_write);
}

int32_t meshx_msg_queue_receive(meshx_msg_queue_t msg_queue, void *pmsg, uint32_t wait_ms)
{
    meshx_msg_queue_wrapper_t *pmsg_queue = (meshx_msg_queue_wrapper_t *)msg_queue;
    read(pmsg_queue->fd_read, pmsg, pmsg_queue->msg_size);
    return MESHX_SUCCESS;
}

int32_t meshx_msg_queue_send(meshx_msg_queue_t msg_queue, void *pmsg)
{
    meshx_msg_queue_wrapper_t *pmsg_queue = (meshx_msg_queue_wrapper_t *)msg_queue;
    write(pmsg_queue->fd_read, pmsg, pmsg_queue->msg_size);
    return MESHX_SUCCESS;
}

