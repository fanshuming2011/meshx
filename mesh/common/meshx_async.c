/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_ASYNC"
#include "meshx_trace.h"
#include "meshx_async.h"
#include "meshx_async_internal.h"
#include "meshx_provision_internal.h"
#include "meshx_errno.h"

meshx_async_msg_notify_t meshx_async_msg_notify;

int32_t meshx_async_msg_set_notify(meshx_async_msg_notify_t handler)
{
    meshx_async_msg_notify = handler;

    return MESHX_SUCCESS;
}

void meshx_async_msg_process(meshx_async_msg_t *pmsg)
{
    if (NULL == pmsg)
    {
        MESHX_ERROR("invalid async message");
        return ;
    }

    switch (pmsg->type)
    {
    case MESHX_ASYNC_MSG_TYPE_TIMEOUT_PB_ADV_RETRY:
    case MESHX_ASYNC_MSG_TYPE_TIMEOUT_PB_ADV_LINK_LOSS:
        meshx_pb_adv_async_handle_timeout(pmsg);
        break;
    default:
        MESHX_ERROR("unkonwn message type: %d", pmsg->type);
        break;
    }
}
