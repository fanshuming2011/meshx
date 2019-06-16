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

static meshx_timer_t beacon_timer;

int32_t meshx_beacon_start(meshx_bearer_t bearer, meshx_beacon_type_t type, uint32_t interval)
{
    if (MESHX_BEARER_TYPE_ADV != bearer->type)
    {
        MESHX_ERROR("beacon can only send on advertising bearer!");
        return -MESHX_ERR_INVAL;
    }
    int32_t ret = MESHX_SUCCESS;
    switch (type)
    {
    case MESHX_BEACON_TYPE_UDB:
        if (MESHX_ADDRESS_UNASSIGNED != meshx_node_address())
        {
            ret = -MESHX_ERR_STATE;
        }
        else
        {
        }
        break;
    case MESHX_BEACON_TYPE_SNB:
        if (MESHX_ADDRESS_UNASSIGNED == meshx_node_address())
        {
            ret = -MESHX_ERR_STATE;
        }
        else
        {
        }
        break;
    default:
        MESHX_ERROR("invalid beacon type: %d", type)
        ret = -MESHX_ERR_INVAL;
        break;
    }
    return ret;
}

void meshx_beacon_stop(meshx_beacon_type_t type)
{
}

