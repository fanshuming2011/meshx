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
    meshx_app_key_init();
    meshx_net_key_init();

    meshx_bearer_init();
    if (meshx_node_params.config.adv_bearer_enable)
    {
        meshx_gap_init(meshx_node_params.config.gap_task_num);
    }
    meshx_network_init();
    meshx_lower_transport_init();
    meshx_provision_init();

    meshx_bearer_param_t adv_param = {.bearer_type = MESHX_BEARER_TYPE_ADV};
    meshx_bearer_t adv_bearer = meshx_bearer_create(adv_param);

    meshx_network_if_t adv_network_if = meshx_network_if_create();
    meshx_network_if_connect(adv_network_if, adv_bearer, NULL, NULL);


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
        meshx_beacon_start(meshx_bearer_adv_get(), MESHX_BEACON_TYPE_UDB,
                           meshx_node_params.param.udb_interval);
    }
    return MESHX_SUCCESS;
}

