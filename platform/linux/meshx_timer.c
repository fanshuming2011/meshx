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
#define MESHX_TRACE_MODULE "MESHX_TIMER"
#include "meshx_trace.h"
#include "meshx_timer.h"
#include "meshx_errno.h"
#include "meshx_list.h"
#include "meshx_mem.h"

typedef struct
{
    timer_t timer;
    meshx_timer_mode_t mode;
    meshx_timer_handler_t timeout_handler;
    void *pargs;
} meshx_timer_wrapper_t;

void timer_handle_thread(union sigval sig)
{
    meshx_timer_wrapper_t *ptimer_wrapper = sig.sival_ptr;
    ptimer_wrapper->timeout_handler(ptimer_wrapper->pargs);
}

int32_t meshx_timer_create(meshx_timer_t *ptimer, meshx_timer_mode_t mode,
                           meshx_timer_handler_t phandler, void *pargs)
{
    struct sigevent evp;
    memset(&evp, 0, sizeof(struct sigevent));

    meshx_timer_wrapper_t *ptimer_wrapper = meshx_malloc(sizeof(meshx_timer_wrapper_t));
    if (NULL == ptimer_wrapper)
    {
        MESHX_ERROR("create timer failed: out of memory");
        return -MESHX_ERR_MEM;
    }

    evp.sigev_value.sival_ptr = ptimer_wrapper;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = timer_handle_thread;

    timer_t timer_id;
    if (timer_create(CLOCK_REALTIME, &evp, &timer_id) == -1)
    {
        meshx_free(ptimer_wrapper);
        MESHX_ERROR("create timer failed: internal");
        return -MESHX_ERR_FAIL;
    }
    *ptimer = ptimer_wrapper;

    ptimer_wrapper->timer = timer_id;
    ptimer_wrapper->mode = mode;
    ptimer_wrapper->timeout_handler = phandler;
    ptimer_wrapper->pargs = pargs;

    return MESHX_SUCCESS;
}

int32_t meshx_timer_start(meshx_timer_t timer, uint32_t interval)
{
    meshx_timer_wrapper_t *ptimer_wrapper = (meshx_timer_wrapper_t *)timer;
    if (NULL == ptimer_wrapper)
    {
        MESHX_ERROR("invalid timer: 0x%x", timer);
        return -MESHX_ERR_INVAL;
    }

    struct itimerspec it;
    if (MESHX_TIMER_MODE_SINGLE_SHOT == ptimer_wrapper->mode)
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
        it.it_value.tv_sec = it.it_interval.tv_sec;
        it.it_value.tv_nsec = it.it_interval.tv_nsec;
    }

    if (timer_settime(ptimer_wrapper->timer, 0, &it, NULL) == -1)
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
    meshx_timer_wrapper_t *ptimer_wrapper = (meshx_timer_wrapper_t *)timer;
    if (NULL == ptimer_wrapper)
    {
        MESHX_ERROR("invalid timer: 0x%x", timer);
        return ;
    }

    timer_delete(ptimer_wrapper->timer);
    meshx_free(ptimer_wrapper);
}

bool meshx_timer_is_active(meshx_timer_t timer)
{
    meshx_timer_wrapper_t *ptimer_wrapper = (meshx_timer_wrapper_t *)timer;
    if (NULL == ptimer_wrapper)
    {
        MESHX_ERROR("invalid timer: 0x%x", timer);
        return FALSE;
    }
    struct itimerspec curr_val;
    if (-1 == timer_gettime(ptimer_wrapper->timer, &curr_val))
    {
        MESHX_ERROR("failed to get time");
        return FALSE;
    }

    return ((0 == curr_val.it_value.tv_sec) && (0 == curr_val.it_value.tv_nsec));
}