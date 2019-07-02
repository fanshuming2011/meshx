/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_NODE"
#include "meshx_trace.h"
#include "meshx_node.h"
#include "meshx_errno.h"

static meshx_node_provision_state_t prov_state;

typedef struct
{
    meshx_dev_uuid_t dev_uuid;
    uint32_t udb_interval;
    uint32_t snb_interval;
} meshx_node_params_t;

static meshx_node_params_t node_params;

int32_t meshx_node_param_set(const meshx_node_param_t *pparam)
{
    if (NULL == pparam)
    {
        return -MESHX_ERR_INVAL;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (pparam->type)
    {
    case MESHX_NODE_PARAM_TYPE_DEV_UUID:
        memcpy(node_params.dev_uuid, pparam->dev_uuid, sizeof(meshx_dev_uuid_t));
        break;
    case MESHX_NODE_PARAM_TYPE_UDB_INTERVAL:
        node_params.udb_interval = pparam->udb_interval;
        break;
    case MESHX_NODE_PARAM_TYPE_SNB_INTERVAL:
        node_params.snb_interval = pparam->snb_interval;
        break;
    default:
        MESHX_WARN("unknown parameter type: %d", pparam->type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

int32_t meshx_node_param_get(meshx_node_param_type_t type, void *pdata)
{
    if (NULL == pdata)
    {
        return -MESHX_ERR_INVAL;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (type)
    {
    case MESHX_NODE_PARAM_TYPE_DEV_UUID:
        memcpy(pdata, node_params.dev_uuid, sizeof(meshx_dev_uuid_t));
        break;
    case MESHX_NODE_PARAM_TYPE_UDB_INTERVAL:
        *((uint32_t *)pdata) = node_params.udb_interval;
        break;
    case MESHX_NODE_PARAM_TYPE_SNB_INTERVAL:
        *((uint32_t *)pdata) = node_params.snb_interval;
        break;
    default:
        MESHX_WARN("unknown parameter type: %d", type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

uint16_t meshx_get_node_address(void)
{
    return MESHX_ADDRESS_UNASSIGNED;
}

void meshx_set_node_prov_state(meshx_node_provision_state_t state)
{
    prov_state = state;
}

meshx_node_provision_state_t meshx_get_node_prov_state(void)
{
    return prov_state;
}
