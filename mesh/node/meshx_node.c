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
#include "meshx_mem.h"
#include "meshx_list.h"

static meshx_node_prov_state_t prov_state;
typedef struct
{
    meshx_network_key_t net_key;
    meshx_list_t node;
} meshx_net_key_info_t;

typedef struct
{
    meshx_application_key_t app_key;
    meshx_list_t node;
} meshx_app_key_info_t;

static meshx_list_t meshx_net_keys = {.pprev = &meshx_net_keys, .pnext = &meshx_net_keys};
static meshx_list_t meshx_app_keys = {.pprev = &meshx_app_keys, .pnext = &meshx_app_keys};

typedef struct
{
    meshx_dev_uuid_t dev_uuid;
    uint32_t udb_interval;
    uint32_t snb_interval;
    meshx_dev_key_t dev_key;
    uint16_t net_key_num;
    uint16_t app_key_num;
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
    case MESHX_NODE_PARAM_TYPE_NET_KEY_NUM:
        node_params.net_key_num = pparam->net_key_num;
        break;
    case MESHX_NODE_PARAM_TYPE_APP_KEY_NUM:
        node_params.app_key_num = pparam->app_key_num;
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

uint16_t meshx_node_address_get(void)
{
    return MESHX_ADDRESS_UNASSIGNED;
}

void meshx_node_prov_state_set(meshx_node_prov_state_t state)
{
    prov_state = state;
}

meshx_node_prov_state_t meshx_node_prov_state_get(void)
{
    return prov_state;
}

meshx_net_key_info_t *meshx_find_net_key(uint16_t net_key_index)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key = NULL;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnet_key, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            break;
        }
    }

    return pnet_key;
}

const meshx_application_key_t *meshx_app_key_get(uint16_t app_key_index)
{
    meshx_list_t *pnode;
    meshx_app_key_info_t *papp_key;
    meshx_list_foreach(pnode, &meshx_app_keys)
    {
        papp_key = MESHX_CONTAINER_OF(papp_key, meshx_app_key_info_t, node);
        if (papp_key->app_key.app_key_index == app_key_index)
        {
            return &papp_key->app_key;
        }
    }

    return NULL;
}

int32_t meshx_node_app_key_add(uint16_t net_key_index, uint16_t app_key_index,
                               meshx_app_key_t app_key)
{
    meshx_list_t *pnode;
    meshx_app_key_info_t *papp_key;
    meshx_list_foreach(pnode, &meshx_app_keys)
    {
        papp_key = MESHX_CONTAINER_OF(papp_key, meshx_app_key_info_t, node);
        if (papp_key->app_key.app_key_index == app_key_index)
        {
            MESHX_ERROR("app key index alreay used: %d", app_key_index);
            return -MESHX_ERR_ALREADY;
        }
    }

    if (meshx_list_length(&meshx_app_keys) >= node_params.app_key_num)
    {
        MESHX_ERROR("net key num reaches maximum number: %d", node_params.net_key_num);
        return -MESHX_ERR_RESOURCE;
    }

    meshx_net_key_info_t *pnet_key = meshx_find_net_key(net_key_index);
    if (NULL == pnet_key)
    {
        MESHX_ERROR("invalid net key index: %d", net_key_index);
        return -MESHX_ERR_NOT_FOUND;
    }

    papp_key = meshx_malloc(sizeof(meshx_app_key_info_t));
    if (NULL == papp_key)
    {
        MESHX_ERROR("can not add app key: out of memory");
        return -MESHX_ERR_MEM;
    }

    papp_key->app_key.app_key_index = app_key_index;
    memcpy(papp_key->app_key.app_key, app_key, sizeof(meshx_app_key_t));
    papp_key->app_key.pnet_key_bind = &pnet_key->net_key;
    meshx_list_append(&meshx_net_keys, &pnet_key->node);

    return MESHX_SUCCESS;
}

int32_t meshx_node_app_key_update(uint16_t net_key_index, uint16_t app_key_index,
                                  meshx_app_key_t app_key)
{

    return MESHX_SUCCESS;
}

int32_t meshx_node_app_key_delte(uint16_t net_key_index, uint16_t app_key_index)
{

    return MESHX_SUCCESS;
}

void meshx_node_app_key_clear(void)
{
}

const meshx_network_key_t *meshx_net_key_get(uint16_t net_key_index)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnet_key, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            return &pnet_key->net_key;
        }
    }

    return NULL;
}

int32_t meshx_net_key_add(uint16_t net_key_index, meshx_net_key_t net_key)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnet_key, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            MESHX_ERROR("net key index alreay used: %d", net_key_index);
            return -MESHX_ERR_ALREADY;
        }
    }

    if (meshx_list_length(&meshx_net_keys) >= node_params.net_key_num)
    {
        MESHX_ERROR("net key num reaches maximum number: %d", node_params.net_key_num);
        return -MESHX_ERR_RESOURCE;
    }

    pnet_key = meshx_malloc(sizeof(meshx_net_key_info_t));
    if (NULL == pnet_key)
    {
        MESHX_ERROR("can not add net key: out of memory");
        return -MESHX_ERR_MEM;
    }

    pnet_key->net_key.net_key_index = net_key_index;
    memcpy(pnet_key->net_key.net_key, net_key, sizeof(meshx_net_key_t));
    meshx_list_append(&meshx_net_keys, &pnet_key->node);

    return MESHX_SUCCESS;
}

int32_t meshx_net_key_update(uint16_t net_key_index, meshx_net_key_t net_key)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnet_key, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            /* TODO:need compare or process app key? */
            memcpy(pnet_key->net_key.net_key, net_key, sizeof(meshx_net_key_t));
            return MESHX_SUCCESS;
        }
    }

    return -MESHX_ERR_NOT_FOUND;
}

int32_t meshx_node_delete_net_key(uint16_t net_key_index)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnet_key, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            meshx_list_remove(pnode);
            break;
        }
    }

    if (NULL == pnet_key)
    {
        return -MESHX_ERR_NOT_FOUND;
    }

    /* TODO: update all bind app keys? */
    meshx_free(pnet_key);

    return MESHX_SUCCESS;

}

void meshx_node_net_key_clear(void)
{
}