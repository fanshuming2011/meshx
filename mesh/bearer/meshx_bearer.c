/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "MESHX_BEARER"
#include "meshx_bearer.h"
#include "meshx_bearer_internal.h"
#include "meshx_errno.h"
#include "meshx_trace.h"
#include "meshx_assert.h"
#include "meshx_gap.h"
#include "meshx_net.h"
#include "meshx_node_internal.h"


int32_t meshx_bearer_init(void)
{
    if (meshx_node_params.config.adv_bearer_enable)
    {
        meshx_bearer_adv_init();
    }
    else
    {
        MESHX_WARN("adv bearer is diabled!");
    }

    if (meshx_node_params.config.gatt_bearer_enable)
    {
        meshx_bearer_gatt_init();
    }
    else
    {
        MESHX_WARN("gatt bearer is diabled!");
    }

    return MESHX_SUCCESS;
}

uint8_t meshx_bearer_type_get(meshx_bearer_t bearer)
{
    return (NULL == bearer) ? MESHX_BEARER_TYPE_INVALID : bearer->type;
}

void meshx_bearer_delete(meshx_bearer_t bearer)
{
    if (NULL != bearer)
    {
        switch (bearer->type)
        {
        case MESHX_BEARER_TYPE_ADV:
            meshx_bearer_adv_delete(bearer);
            break;
        case MESHX_BEARER_TYPE_GATT:
            meshx_bearer_gatt_delete(bearer);
            break;
        default:
            MESHX_WARN("invalid bearer type: %d", bearer->type);
            break;
        }
    }
}

