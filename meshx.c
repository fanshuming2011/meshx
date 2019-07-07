/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx.h"
#include "meshx_bearer_internal.h"


static meshx_config_t default_config =
{
    .dev_uuid = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    .adv_bearer_enable = TRUE,
    .gatt_bearer_enable = TRUE,
    .udb_interval = 5000,
    .snb_interval = 10000,
};

static uint8_t meshx_role = MESHX_ROLE_DEVICE;
static meshx_config_t config;

void meshx_config(uint8_t role, const meshx_config_t *pconfig)
{
    meshx_role = role;
    if (NULL == pconfig)
    {
        meshx_mac_addr_t addr;
        meshx_gap_get_mac_addr(addr);
        memcpy(default_config.dev_uuid, addr, sizeof(meshx_mac_addr_t));
        config = default_config;
    }
    else
    {
        config = *pconfig;
    }
}

meshx_config_t meshx_get_default_config(void)
{
    meshx_mac_addr_t addr;
    meshx_gap_get_mac_addr(addr);
    memcpy(default_config.dev_uuid, addr, sizeof(meshx_mac_addr_t));
    return default_config;
}

int32_t meshx_init(void)
{
    meshx_node_param_t node_param;

    node_param.type = MESHX_NODE_PARAM_TYPE_DEV_UUID;
    memcpy(node_param.dev_uuid, config.dev_uuid, sizeof(meshx_dev_uuid_t));
    meshx_node_param_set(&node_param);

    node_param.type = MESHX_NODE_PARAM_TYPE_UDB_INTERVAL;
    node_param.udb_interval = config.udb_interval;
    meshx_node_param_set(&node_param);

    node_param.type = MESHX_NODE_PARAM_TYPE_SNB_INTERVAL;
    node_param.snb_interval = config.snb_interval;
    meshx_node_param_set(&node_param);

    meshx_trace_init();
    meshx_trace_level_enable(MESHX_TRACE_LEVEL_ALL);

    meshx_bearer_init(config.adv_bearer_enable, config.gatt_bearer_enable);
    if (config.adv_bearer_enable)
    {
        meshx_gap_init();
    }
    meshx_network_init();
    meshx_provision_init();

    meshx_bearer_param_t adv_param = {.bearer_type = MESHX_BEARER_TYPE_ADV, .param_adv.adv_period = 0};
    meshx_bearer_t adv_bearer = meshx_bearer_create(adv_param);

    meshx_network_if_t adv_network_if = meshx_network_if_create();
    meshx_network_if_connect(adv_network_if, adv_bearer, NULL, NULL);

    return MESHX_SUCCESS;
}

int32_t meshx_run(void)
{
    if (config.adv_bearer_enable)
    {
        meshx_gap_start();
    }

    if (meshx_role & MESHX_ROLE_DEVICE)
    {
        meshx_beacon_start(meshx_bearer_adv_get(), MESHX_BEACON_TYPE_UDB, 5000);//config.udb_interval);
    }
    return MESHX_SUCCESS;
}

