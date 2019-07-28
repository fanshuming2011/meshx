/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_cmd_prov.h"
#include "meshx.h"
#include "meshx_list.h"


bool meshx_show_beacon;
static meshx_provision_dev_t prov_dev = NULL;
static meshx_list_t prov_dev_list = {.pprev = &prov_dev_list, .pnext = &prov_dev_list};
typedef struct
{
    meshx_provision_dev_t prov_dev;
    meshx_list_t node;
} meshx_prov_devs_t;

int32_t meshx_cmd_prov_add_device(meshx_provision_dev_t prov_dev)
{
    meshx_prov_devs_t *pdev = meshx_malloc(sizeof(meshx_prov_devs_t));
    if (NULL == pdev)
    {
        MESHX_ERROR("add provison device failed!");
        return -MESHX_ERR_MEM;
    }
    pdev->prov_dev = prov_dev;
    meshx_list_append(&prov_dev_list, &pdev->node);
    return MESHX_SUCCESS;
}

void meshx_cmd_prov_remove_device(meshx_provision_dev_t prov_dev)
{
    meshx_list_t *pnode;
    meshx_prov_devs_t *pdev = NULL;
    meshx_list_foreach(pnode, &prov_dev_list)
    {
        pdev = MESHX_CONTAINER_OF(pnode, meshx_prov_devs_t, node);
        if (pdev->prov_dev == prov_dev)
        {
            meshx_list_remove(pnode);
            return ;
        }
    }
}

static meshx_provision_dev_t meshx_cmd_prov_get_device(uint32_t id)
{
    meshx_list_t *pnode;
    meshx_prov_devs_t *pdev = NULL;
    meshx_list_foreach(pnode, &prov_dev_list)
    {
        pdev = MESHX_CONTAINER_OF(pnode, meshx_prov_devs_t, node);
        if (meshx_provision_get_device_id(pdev->prov_dev) == id)
        {
            return pdev->prov_dev;
        }
    }

    return NULL;
}

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
    uint32_t id = pparsed_data->param_val[0];
    meshx_provision_dev_t prov_dev = meshx_cmd_prov_get_device(id);
    if (NULL == prov_dev)
    {
        return -MESHX_ERR_NOT_FOUND;
    }
    meshx_provision_invite_t invite = {pparsed_data->param_val[1]};
    meshx_provision_invite(prov_dev, invite);

    return MESHX_SUCCESS;
}

int32_t meshx_cmd_prov_capabilites(const meshx_cmd_parsed_data_t *pparsed_data)
{
    uint32_t id = pparsed_data->param_val[0];
    meshx_provision_dev_t prov_dev = meshx_cmd_prov_get_device(id);
    if (NULL == prov_dev)
    {
        return -MESHX_ERR_NOT_FOUND;
    }

    meshx_provision_capabilites_t capbilites;
    capbilites.element_nums = pparsed_data->param_val[1];
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
    uint32_t id = pparsed_data->param_val[0];
    meshx_provision_dev_t prov_dev = meshx_cmd_prov_get_device(id);
    if (NULL == prov_dev)
    {
        return -MESHX_ERR_NOT_FOUND;
    }

    meshx_provision_start_t start;
    start.algorithm = 0;
    start.public_key = pparsed_data->param_val[1];
    start.auth_method = pparsed_data->param_val[2];
    start.auth_action = pparsed_data->param_val[3];
    start.auth_size = pparsed_data->param_val[4];
    meshx_provision_start(prov_dev, &start);

    return MESHX_SUCCESS;
}

int32_t meshx_cmd_prov_set_public_key(const meshx_cmd_parsed_data_t *pparsed_data)
{
    uint32_t id = pparsed_data->param_val[0];
    meshx_provision_dev_t prov_dev = meshx_cmd_prov_get_device(id);
    if (NULL == prov_dev)
    {
        return -MESHX_ERR_NOT_FOUND;
    }

    meshx_provision_public_key_t pub_key;
    meshx_bin2hex(pparsed_data->param_ptr[1], (uint8_t *)&pub_key,
                  sizeof(meshx_provision_public_key_t) * 2);

    return meshx_provision_set_remote_public_key(prov_dev, &pub_key);
}

int32_t meshx_cmd_prov_public_key(const meshx_cmd_parsed_data_t *pparsed_data)
{
    uint32_t id = pparsed_data->param_val[0];
    meshx_provision_dev_t prov_dev = meshx_cmd_prov_get_device(id);
    if (NULL == prov_dev)
    {
        return -MESHX_ERR_NOT_FOUND;
    }

    meshx_provision_public_key_t pub_key;
    meshx_provision_get_local_public_key(prov_dev, &pub_key);
    meshx_provision_public_key(prov_dev, &pub_key);

    return MESHX_SUCCESS;
}
