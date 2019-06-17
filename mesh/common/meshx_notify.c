/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_NOTIFY"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_notify.h"

static meshx_notify_t notify;

static int32_t notify_default(uint8_t notify_type, const void *pdata, uint8_t len)
{
    return MESHX_SUCCESS;
}

int32_t meshx_notify_init(meshx_notify_t notify_func)
{
    if (NULL == notify_func)
    {
        MESHX_INFO("use default notify function");
        notify = notify_default;
    }
    else
    {
        MESHX_INFO("use user notify function");
        notify = notify_func;
    }

    return MESHX_SUCCESS;
}

int32_t meshx_notify(uint8_t notify_type, const void *pdata, uint8_t len)
{
    return notify(notify_type, pdata, len);
}