/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_TIMER_H_
#define _MESHX_TIMER_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef void *meshx_timer_t;
typedef void (*meshx_timer_handler_t)(void *pargs);

typedef enum
{
    MESHX_TIMER_MODE_SINGLE_SHOT,
    MESHX_TIMER_MODE_REPEATED,
} meshx_timer_mode_t;

int32_t meshx_timer_create(meshx_timer_t *ptimer, meshx_timer_mode_t mode,
                           meshx_timer_handler_t phandler, void *pargs);
int32_t meshx_timer_start(meshx_timer_t timer, uint32_t interval);
int32_t meshx_timer_stop(meshx_timer_t timer);
void meshx_timer_delete(meshx_timer_t timer);
bool meshx_timer_is_active(meshx_timer_t timer);

MESHX_END_DECLS



#endif
