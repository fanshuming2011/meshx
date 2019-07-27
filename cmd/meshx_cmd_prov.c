/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_cmd_prov.h"
#include "meshx.h"


bool meshx_show_beacon;
static meshx_provision_dev_t prov_dev = NULL;

int32_t meshx_cmd_prov_scan(const meshx_cmd_parsed_data_t *pparsed_data)
{
    meshx_show_beacon = pparsed_data->param_val[0];
    return MESHX_SUCCESS;
}

int32_t meshx_cmd_prov_conn(const meshx_cmd_parsed_data_t *pparsed_data)
{
    meshx_bearer_rx_metadata_t rx_meatadata = {.bearer_type = MESHX_BEARER_TYPE_ADV};
    meshx_dev_uuid_t dev_uuid;
    meshx_bin2hex(pparsed_data->param_ptr[0], dev_uuid, sizeof(meshx_dev_uuid_t) * 2);
    prov_dev = meshx_provision_create_device(meshx_bearer_get(&rx_meatadata),
                                             dev_uuid, MESHX_ROLE_PROVISIONER);
    meshx_provision_link_open(prov_dev);
    return MESHX_SUCCESS;
}

int32_t meshx_cmd_prov_invite(const meshx_cmd_parsed_data_t *pparsed_data)
{
    meshx_provision_invite_t invite = {pparsed_data->param_val[0]};
    meshx_provision_invite(prov_dev, invite);

    return MESHX_SUCCESS;
}

int32_t meshx_cmd_prov_capabilites(const meshx_cmd_parsed_data_t *pparsed_data)
{
    meshx_provision_capabilites_t capbilites;
    capbilites.element_nums = pparsed_data->param_val[0];
    capbilites.algorithms = 1;
    capbilites.public_key_type = pparsed_data->param_val[2];
    capbilites.static_oob_type = pparsed_data->param_val[3];
    capbilites.output_oob_size = pparsed_data->param_val[4];
    capbilites.output_oob_action = pparsed_data->param_val[5];
    capbilites.input_oob_size = pparsed_data->param_val[6];
    capbilites.input_oob_action = pparsed_data->param_val[7];
    meshx_provision_capabilites(prov_dev, &capbilites);

    return MESHX_SUCCESS;
}

int32_t meshx_cmd_prov_start(const meshx_cmd_parsed_data_t *pparsed_data)
{
    meshx_provision_start_t start;
    start.algorithm = pparsed_data->param_val[0];
    start.public_key = pparsed_data->param_val[1];
    start.auth_method = pparsed_data->param_val[2];
    start.auth_action = pparsed_data->param_val[3];
    start.auth_size = pparsed_data->param_val[4];
    meshx_provision_start(prov_dev, &start);

    return MESHX_SUCCESS;
}
