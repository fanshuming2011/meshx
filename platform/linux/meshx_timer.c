/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define TRACE_MODULE "MESHX_TIMER"
#include "meshx_trace.h"
#include "meshx_timer.h"
#include "meshx_errno.h"
#include "meshx_list.h"
#include "meshx_mem.h"

typedef struct
{
    meshx_timer_t timer;
    meshx_timer_mode_t mode;
    meshx_timer_handler_t timeout_handler;
    meshx_list_t node;
} meshx_timer_info_t;

static meshx_list_t timer_list = {.pprev = &timer_list, .pnext = &timer_list};

static meshx_timer_info_t *meshx_timer_info_get(meshx_timer_t timer)
{
    meshx_list_t *pnode;
    meshx_timer_info_t *ptimer_info;
    meshx_list_foreach(pnode, &timer_list)
    {
        ptimer_info = MESHX_CONTAINER_OF(pnode, meshx_timer_info_t, node);
        if (ptimer_info->timer == timer)
        {
            return ptimer_info;
        }
    }

    return NULL;
}

void timer_handle_thread(union sigval sig)
{
    meshx_timer_t *ptimer = sig.sival_ptr;
    meshx_timer_info_t *ptimer_info = meshx_timer_info_get(*ptimer);
    if (NULL == ptimer_info)
    {
        MESHX_WARN("invalid timer: 0x%x", *ptimer);
    }

    ptimer_info->timeout_handler(*ptimer);
}

int32_t meshx_timer_create(meshx_timer_t *ptimer, meshx_timer_mode_t mode,
                           meshx_timer_handler_t phandler)
{
    struct sigevent evp;
    memset(&evp, 0, sizeof(struct sigevent));

    evp.sigev_value.sival_ptr = ptimer;
    evp.sigev_notify = SIGEV_THREAD;        //线程通知的方式，派驻新线程
    evp.sigev_notify_function = timer_handle_thread;   //线程函数地址

    if (timer_create(CLOCK_REALTIME, &evp, ptimer) == -1)
    {
        MESHX_ERROR("create timer failed: internal");
        return MESHX_ERR_FAIL;
    }

    meshx_timer_info_t *ptimer_info = meshx_malloc(sizeof(meshx_timer_info_t));
    if (NULL == ptimer_info)
    {
        timer_delete(*ptimer);
        MESHX_ERROR("create timer failed: out of memory");
        return MESHX_ERR_NO_MEM;
    }
    ptimer_info->timer = *ptimer;
    ptimer_info->mode = mode;
    ptimer_info->timeout_handler = phandler;

    meshx_list_append(&timer_list, &ptimer_info->node);
    return MESHX_SUCCESS;
}

int32_t meshx_timer_start(meshx_timer_t timer, uint32_t interval)
{
    meshx_timer_info_t *ptimer_info = meshx_timer_info_get(timer);
    if (NULL == ptimer_info)
    {
        MESHX_ERROR("invalid timer: 0x%x", timer);
        return -MESHX_ERR_INVAL;
    }

    struct itimerspec it;
    if (MESHX_TIMER_SINGLE_SHOT == ptimer_info->mode)
    {
        it.it_interval.tv_sec = 0;
        it.it_interval.tv_nsec = 0;
        it.it_value.tv_sec = interval / 1000;
        it.it_value.tv_nsec = (interval % 1000) * 1000000;
    }
    else
    {
        it.it_interval.tv_sec = interval / 1000;
        it.it_interval.tv_nsec = (interval % 1000) * 1000000;
        it.it_value.tv_sec = interval / 1000;
        it.it_value.tv_nsec = (interval % 1000) * 1000000;
    }

    if (timer_settime(ptimer_info->timer, 0, &it, NULL) == -1)
    {
        MESHX_ERROR("failed to set timer");
        return -MESHX_ERR_FAIL;
    }

    return MESHX_SUCCESS;
}

int32_t meshx_timer_change_interval(meshx_timer_t timer, uint32_t interval)
{
    return meshx_timer_start(timer, interval);
}

int32_t meshx_timer_stop(meshx_timer_t timer)
{
    return meshx_timer_start(timer, 0);
}

void meshx_timer_delete(meshx_timer_t timer)
{
    meshx_timer_info_t *ptimer_info = meshx_timer_info_get(timer);
    if (NULL == ptimer_info)
    {
        MESHX_ERROR("invalid timer: 0x%x", timer);
        return ;
    }

    meshx_list_remove(&ptimer_info->node);
    meshx_free(ptimer_info);
    timer_delete(timer);
}

bool meshx_timer_is_active(meshx_timer_t timer)
{
    meshx_timer_info_t *ptimer_info = meshx_timer_info_get(timer);
    if (NULL == ptimer_info)
    {
        MESHX_ERROR("invalid timer: 0x%x", timer);
        return FALSE;
    }
    struct itimerspec curr_val;
    if (-1 == timer_gettime(timer, &curr_val))
    {
        MESHX_ERROR("failed to get time");
        return FALSE;
    }

    return ((0 == curr_val.it_value.tv_sec) && (0 == curr_val.it_value.tv_nsec));
}