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
#include "meshx_node_internal.h"


meshx_node_params_t meshx_node_params;

static meshx_node_config_t node_default_config =
{
    .role = MESHX_ROLE_DEVICE,
    .dev_uuid = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    .adv_bearer_enable = TRUE,
    .gatt_bearer_enable = TRUE,
    .net_key_num = 2,
    .app_key_num = 2,
    .dev_key_num = 10,
    .nmc_size = 64,
    .rpl_size = 16,
    .gap_task_num = 20,
    .trans_tx_task_num = 3,
    .trans_rx_task_num = 3,
    .trans_tx_retry_times = 1,
};

static meshx_node_param_t node_default_param =
{
    .node_addr = MESHX_ADDRESS_UNASSIGNED,
    .udb_interval = 50,
    .snb_interval = 100,
    .trans_retrans_count = 2,
    .default_ttl = 5,
};

int32_t meshx_node_config_init(meshx_node_config_t *pconfig)
{
    if (NULL == pconfig)
    {
        MESHX_ERROR("invalid config file!");
        return -MESHX_ERR_INVAL;
    }

    *pconfig = node_default_config;
    meshx_mac_addr_t addr;
    meshx_gap_get_mac_addr(addr);
    memcpy(pconfig->dev_uuid, addr, sizeof(meshx_mac_addr_t));

    return MESHX_SUCCESS;
}

int32_t meshx_node_config_set(const meshx_node_config_t *pconfig)
{
    if (NULL == pconfig)
    {
        MESHX_ERROR("invalid config file");
        return -MESHX_ERR_INVAL;
    }

    meshx_node_params.config = *pconfig;

    return MESHX_SUCCESS;
}

meshx_node_config_t meshx_node_config_get(void)
{
    return meshx_node_params.config;
}

int32_t meshx_node_params_init(meshx_node_param_t *pparam)
{
    if (NULL == pparam)
    {
        MESHX_ERROR("invalid parameters file!");
        return -MESHX_ERR_INVAL;
    }

    *pparam = node_default_param;

    return MESHX_SUCCESS;
}

int32_t meshx_node_params_set(const meshx_node_param_t *pparam)
{
    if (NULL == pparam)
    {
        MESHX_ERROR("invalid parameters file!");
        return -MESHX_ERR_INVAL;
    }

    meshx_node_params.param = *pparam;

    return MESHX_SUCCESS;
}

meshx_node_param_t meshx_node_params_get(void)
{
    return meshx_node_params.param;
}

int32_t meshx_node_param_set(meshx_node_param_type_t type, const void *pdata)
{
    int32_t ret = MESHX_SUCCESS;
    switch (type)
    {
    case MESHX_NODE_PARAM_TYPE_NODE_ADDR:
        {
            uint16_t addr = *(uint16_t *)pdata;
            if (MESHX_ADDRESS_IS_UNICAST(addr))
            {
                meshx_node_params.param.node_addr = addr;
            }
        }
        break;
    case MESHX_NODE_PARAM_TYPE_UDB_INTERVAL:
        {
            meshx_node_params.param.udb_interval = (*(uint32_t *)pdata) * 100;
        }
        break;
    case MESHX_NODE_PARAM_TYPE_SNB_INTERVAL:
        {
            meshx_node_params.param.snb_interval = (*(uint32_t *)pdata) * 100;
        }
        break;
    case MESHX_NODE_PARAM_TYPE_TRANS_RETRANS_COUNT:
        meshx_node_params.param.trans_retrans_count = *(uint8_t *)pdata;
        break;
    case MESHX_NODE_PARAM_TYPE_DEFAULT_TTL:
        meshx_node_params.param.default_ttl = *(uint8_t *)pdata;
        break;
    default:
        MESHX_WARN("unknown parameter type: %d", type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

int32_t meshx_node_param_get(meshx_node_param_type_t type, void *pdata)
{
    int32_t ret = MESHX_SUCCESS;
    switch (type)
    {
    case MESHX_NODE_PARAM_TYPE_NODE_ADDR:
        *((uint16_t *)pdata) = meshx_node_params.param.node_addr;
        break;
    case MESHX_NODE_PARAM_TYPE_UDB_INTERVAL:
        *((uint32_t *)pdata) = meshx_node_params.param.udb_interval / 100;
        break;
    case MESHX_NODE_PARAM_TYPE_SNB_INTERVAL:
        *((uint32_t *)pdata) = meshx_node_params.param.snb_interval / 100;
        break;
    case MESHX_NODE_PARAM_TYPE_TRANS_RETRANS_COUNT:
        *((uint8_t *)pdata) = meshx_node_params.param.trans_retrans_count;
        break;
    case MESHX_NODE_PARAM_TYPE_DEFAULT_TTL:
        *((uint8_t *)pdata) = meshx_node_params.param.default_ttl;
    default:
        MESHX_WARN("unknown parameter type: %d", type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

bool meshx_node_is_my_address(uint16_t addr)
{
    /* 0xffff, node address, subscribe address, all realy node.... */
    return TRUE;
}

