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

int32_t meshx_init(void)
{
    /* TODO: set to actual element number */
    meshx_seq_init(1);
    meshx_rpl_init();
    meshx_nmc_init();
    meshx_iv_index_init();
    meshx_iv_update_operate_time_set(MESHX_IV_OPERATE_48W);
    meshx_app_key_init();
    meshx_net_key_init();
    meshx_dev_key_init();

    meshx_bearer_init();
    if (meshx_node_params.config.adv_bearer_enable)
    {
        meshx_gap_init(meshx_node_params.config.gap_task_num);
    }
    meshx_net_init();
    meshx_lower_trans_init();
    meshx_upper_trans_init();
    meshx_access_init();
    meshx_prov_init();

    if (meshx_node_params.config.adv_bearer_enable)
    {
        meshx_bearer_param_t adv_param = {.bearer_type = MESHX_BEARER_TYPE_ADV};
        meshx_bearer_t adv_bearer = meshx_bearer_create(adv_param);

        meshx_net_iface_t adv_net_iface = meshx_net_iface_create();
        meshx_net_iface_connect(adv_net_iface, adv_bearer, NULL, NULL);
    }


    return MESHX_SUCCESS;
}

int32_t meshx_run(void)
{
    if (meshx_node_params.config.adv_bearer_enable)
    {
        meshx_gap_start();
    }

    if (meshx_node_params.config.role & MESHX_ROLE_DEVICE)
    {
        if (MESHX_ADDRESS_UNASSIGNED == meshx_node_params.param.node_addr)
        {
            meshx_beacon_start(meshx_bearer_adv_get(), MESHX_BEACON_TYPE_UDB,
                               meshx_node_params.param.udb_interval * MESHX_BEACON_INTERVAL_UNIT);
        }
        else
        {
            meshx_beacon_start(meshx_bearer_adv_get(), MESHX_BEACON_TYPE_SNB,
                               meshx_node_params.param.snb_interval * MESHX_BEACON_INTERVAL_UNIT);
        }
    }
    return MESHX_SUCCESS;
}

