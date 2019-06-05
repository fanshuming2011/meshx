/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#define TRACE_MOULE "MESHX_MSG_QUEUE"
#include "meshx_trace.h"
#include "meshx_msg_queue.h"
#include "meshx_errno.h"
#include "meshx_mem.h"

typedef struct
{
    int fd_read;
    int fd_write;
    int fd_epoll;
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
    if (-1 == pipe2(fd, O_NONBLOCK))
    {
        MESHX_ERROR("create message queue failed: no pipe resource");
        meshx_free(pmsg);
        return MESHX_ERR_RESOURCE;
    }

    int efd = epoll_create(1);
    if (-1 == efd)
    {
        MESHX_ERROR("create message queue failed: no epoll resource");
        close(fd[0]);
        close(fd[1]);
        meshx_free(pmsg);
        return MESHX_ERR_RESOURCE;
    }
    struct epoll_event ev;
    ev.data.fd = fd[0];
    ev.events = EPOLLIN;
    if (-1 == epoll_ctl(efd, EPOLL_CTL_ADD, fd[0], &ev))
    {
        MESHX_ERROR("create message queue failed: configure epoll failed");
        close(fd[0]);
        close(fd[1]);
        meshx_free(pmsg);
        return MESHX_ERR_RESOURCE;
    }

    *pmsg_queue = pmsg;
    pmsg->fd_read = fd[0];
    pmsg->fd_write = fd[1];
    pmsg->fd_epoll = efd;
    pmsg->msg_num = msg_num;
    pmsg->msg_size = msg_size;
    pmsg->fd_epoll = efd;

    return MESHX_SUCCESS;
}

void meshx_msg_queue_delete(meshx_msg_queue_t msg_queue)
{
    meshx_msg_queue_wrapper_t *pmsg = msg_queue;
    close(pmsg->fd_read);
    close(pmsg->fd_write);
    close(pmsg->fd_epoll);
}

int32_t meshx_msg_queue_receive(meshx_msg_queue_t msg_queue, void *pmsg, uint32_t wait_ms)
{
    struct epoll_event ev;
    meshx_msg_queue_wrapper_t *pmsg_queue = (meshx_msg_queue_wrapper_t *)msg_queue;
    if (0 == epoll_wait(pmsg_queue->fd_epoll, &ev, 1, wait_ms))
    {
        return MESHX_ERR_TIMEOUT;
    }
    read(pmsg_queue->fd_read, pmsg, pmsg_queue->msg_size);
    return MESHX_SUCCESS;
}

int32_t meshx_msg_queue_send(meshx_msg_queue_t msg_queue, void *pmsg)
{
    meshx_msg_queue_wrapper_t *pmsg_queue = (meshx_msg_queue_wrapper_t *)msg_queue;
    write(pmsg_queue->fd_read, pmsg, pmsg_queue->msg_size);
    return MESHX_SUCCESS;
}

