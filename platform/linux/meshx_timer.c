/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include "meshx_timer.h"
#include "meshx_errno.h"

int32_t meshx_timer_create(meshx_timer_t *ptimer, meshx_timer_mode_t mode,
                           meshx_timer_handler_t phandler)
{
    return MESHX_SUCCESS;
}

void meshx_timer_start(meshx_timer_t timer, uint32_t interval)
{

}

void meshx_timer_change_interval(meshx_timer_t timer, uint32_t interval)
{

}

void meshx_timer_stop(meshx_timer_t timer)
{

}

void meshx_timer_delete(meshx_timer_t timer)
{

}

bool meshx_is_timer_active(meshx_timer_t timer)
{
    return FALSE;
}