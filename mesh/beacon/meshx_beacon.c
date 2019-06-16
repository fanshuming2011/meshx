/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_beacon.h"
#define TRACE_MODULE "MESHX_BEACON"
#include "meshx_trace.h"
#include "meshx_beacon.h"
#include "meshx_errno.h"
#include "meshx_bearer_internal.h"
#include "meshx_timer.h"
#include "meshx_node.h"
#include "meshx_async_internal.h"

static meshx_timer_t beacon_timer;

static void meshx_beacon_timer_timeout(void *pargs)
{
    meshx_async_msg_t async_msg;
    async_msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_BEACON;
    async_msg.pdata = pargs;
    async_msg.data_len = 0;

    meshx_async_msg_send(&async_msg);
}

void meshx_beacon_async_handle_timeout(meshx_async_msg_t msg)
{
    if (MESHX_ADDRESS_UNASSIGNED == meshx_get_node_address())
    {
        /* send udb */
        //meshx_bearer_send(msg.pdata, MESHX_BEARER_ADV_PKT_TYPE_BEACON, );
    }
    else
    {
        /* send snb */
        //meshx_bearer_send(msg.pdata, MESHX_BEARER_ADV_PKT_TYPE_BEACON, );
    }
}

int32_t meshx_beacon_start(meshx_bearer_t bearer, uint8_t beacon_type, uint32_t interval)
{
    if (MESHX_BEARER_TYPE_ADV != bearer->type)
    {
        MESHX_ERROR("beacon can only send on advertising bearer!");
        return -MESHX_ERR_INVAL;
    }
    int32_t ret = MESHX_SUCCESS;
    switch (beacon_type)
    {
    case MESHX_BEACON_TYPE_UDB:
        if (MESHX_ADDRESS_UNASSIGNED != meshx_get_node_address())
        {
            ret = -MESHX_ERR_STATE;
        }
        break;
    case MESHX_BEACON_TYPE_SNB:
        if (MESHX_ADDRESS_UNASSIGNED == meshx_get_node_address())
        {
            ret = -MESHX_ERR_STATE;
        }
        break;
    default:
        MESHX_ERROR("invalid beacon type: %d", beacon_type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    if (MESHX_SUCCESS == ret)
    {
        if (NULL == beacon_timer)
        {
            ret = meshx_timer_create(&beacon_timer, MESHX_TIMER_MODE_REPEATED, meshx_beacon_timer_timeout,
                                     bearer);
            if (MESHX_SUCCESS == ret)
            {
                meshx_timer_start(beacon_timer, interval);
            }
        }
        else
        {
            if (!meshx_timer_is_active(beacon_timer))
            {
                meshx_timer_start(beacon_timer, interval);
            }
            else
            {
                meshx_timer_change_interval(beacon_timer, interval);
            }
        }
    }

    return ret;
}

void meshx_beacon_stop(uint8_t beacon_type)
{
    if (NULL != beacon_timer)
    {
        meshx_timer_stop(beacon_timer);
        meshx_timer_delete(beacon_timer);
        beacon_timer = NULL;
    }
}

