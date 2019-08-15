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
#include "meshx_node_internal.h"


static meshx_config_t default_config =
{
    .role = MESHX_ROLE_DEVICE,
    .dev_uuid = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    .adv_bearer_enable = TRUE,
    .gatt_bearer_enable = TRUE,
    .udb_interval = 5000,
    .snb_interval = 10000,
    .net_key_num = 2,
    .app_key_num = 2,
    .nmc_size = 64,
    .rpl_size = 16,
};

static uint8_t meshx_role = MESHX_ROLE_DEVICE;
static meshx_config_t config;

int32_t meshx_config_init(meshx_config_t *pconfig)
{
    if (NULL == pconfig)
    {
        MESHX_ERROR("invalid config file!");
        return -MESHX_ERR_INVAL;
    }

    meshx_mac_addr_t addr;
    meshx_gap_get_mac_addr(addr);
    memcpy(default_config.dev_uuid, addr, sizeof(meshx_mac_addr_t));
    *pconfig = default_config;

    return MESHX_SUCCESS;
}

int32_t meshx_config_set(const meshx_config_t *pconfig)
{
    if (NULL == pconfig)
    {
        MESHX_ERROR("invalid config file");
        return -MESHX_ERR_INVAL;
    }
    config = *pconfig;

    memcpy(meshx_node_params.dev_uuid, config.dev_uuid, sizeof(meshx_dev_uuid_t));
    meshx_node_params.udb_interval = config.udb_interval;
    meshx_node_params.snb_interval = config.snb_interval;
    meshx_node_params.net_key_num = config.net_key_num;
    meshx_node_params.app_key_num = config.app_key_num;
    meshx_node_params.nmc_size = config.nmc_size;
    meshx_node_params.rpl_size = config.rpl_size;

    return MESHX_SUCCESS;
}

meshx_config_t meshx_config_get(void)
{
    return config;
}

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
    /* TODO: set to actual element number */
    meshx_seq_init(1);
    meshx_rpl_init(meshx_node_params.rpl_size);
    meshx_nmc_init(meshx_node_params.nmc_size);

    meshx_bearer_init(config.adv_bearer_enable, config.gatt_bearer_enable);
    if (config.adv_bearer_enable)
    {
        meshx_gap_init();
    }
    meshx_network_init();
    meshx_provision_init();

    meshx_bearer_param_t adv_param = {.bearer_type = MESHX_BEARER_TYPE_ADV};
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

