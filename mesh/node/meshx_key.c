/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_KEY"
#include "meshx_key.h"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_list.h"
#include "meshx_security.h"
#include "meshx_node_internal.h"


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

static meshx_list_t meshx_net_keys;
static meshx_list_t meshx_app_keys;


meshx_net_key_info_t *meshx_find_net_key(uint16_t net_key_index)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key = NULL;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            break;
        }
    }

    return pnet_key;
}

int32_t meshx_app_key_init(void)
{
    meshx_list_init_head(&meshx_app_keys);
    return MESHX_SUCCESS;
}

const meshx_application_key_t *meshx_app_key_get(uint16_t app_key_index)
{
    meshx_list_t *pnode;
    meshx_app_key_info_t *papp_key;
    meshx_list_foreach(pnode, &meshx_app_keys)
    {
        papp_key = MESHX_CONTAINER_OF(pnode, meshx_app_key_info_t, node);
        if (papp_key->app_key.app_key_index == app_key_index)
        {
            return &papp_key->app_key;
        }
    }

    return NULL;
}

static void meshx_app_key_derive(meshx_application_key_t *papp_key)
{
    meshx_k4(papp_key->app_key, &papp_key->aid);
    MESHX_DEBUG("aid: 0x%x", papp_key->aid);
}

int32_t meshx_app_key_add(uint16_t net_key_index, uint16_t app_key_index,
                          meshx_key_t app_key)
{
    meshx_list_t *pnode;
    meshx_app_key_info_t *papp_key;
    meshx_list_foreach(pnode, &meshx_app_keys)
    {
        papp_key = MESHX_CONTAINER_OF(pnode, meshx_app_key_info_t, node);
        if (papp_key->app_key.app_key_index == app_key_index)
        {
            MESHX_ERROR("app key index alreay used: %d", app_key_index);
            return -MESHX_ERR_ALREADY;
        }
    }

    if (meshx_list_length(&meshx_app_keys) >= meshx_node_params.config.app_key_num)
    {
        MESHX_ERROR("net key num reaches maximum number: %d", meshx_node_params.config.app_key_num);
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
    memcpy(papp_key->app_key.app_key, app_key, sizeof(meshx_key_t));
    papp_key->app_key.pnet_key_bind = &pnet_key->net_key;
    MESHX_DEBUG("application key: index %d-%d, value ", app_key_index, net_key_index);
    MESHX_DUMP_DEBUG(papp_key->app_key.app_key, sizeof(meshx_key_t));
    meshx_app_key_derive(&papp_key->app_key);
    meshx_list_append(&meshx_app_keys, &papp_key->node);

    return MESHX_SUCCESS;
}

int32_t meshx_app_key_update(uint16_t net_key_index, uint16_t app_key_index,
                             meshx_key_t app_key)
{

    return MESHX_SUCCESS;
}

int32_t meshx_app_key_delete(uint16_t net_key_index, uint16_t app_key_index)
{

    return MESHX_SUCCESS;
}

void meshx_app_key_clear(void)
{
}

int32_t meshx_net_key_init(void)
{
    meshx_list_init_head(&meshx_net_keys);
    return MESHX_SUCCESS;
}

const meshx_network_key_t *meshx_net_key_get(uint16_t net_key_index)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            return &pnet_key->net_key;
        }
    }

    return NULL;
}

const meshx_network_key_t *meshx_net_key_get_by_nid(uint8_t nid)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
        if (pnet_key->net_key.nid == nid)
        {
            return &pnet_key->net_key;
        }
    }

    return NULL;
}

static void meshx_net_key_derive(meshx_network_key_t *pnet_key)
{
    /* identity key */
    uint8_t salt_M[] = {'n', 'k', 'i', 'k'};
    uint8_t salt[16];
    meshx_s1(salt_M, sizeof(salt_M), salt);
    uint8_t P[] = {'i', 'd', '1', '2', '8', 0x01};
    meshx_k1(pnet_key->net_key, sizeof(pnet_key->net_key), salt, P, sizeof(P),
             pnet_key->identity_key);
    MESHX_DEBUG("identity key:");
    MESHX_DUMP_DEBUG(pnet_key->identity_key, sizeof(meshx_key_t));
    /* beacon key */
    salt_M[2] = 'b';
    meshx_s1(salt_M, sizeof(salt_M), salt);
    meshx_k1(pnet_key->net_key, sizeof(pnet_key->net_key), salt, P, sizeof(P),
             pnet_key->beacon_key);
    MESHX_DEBUG("beacon key:");
    MESHX_DUMP_DEBUG(pnet_key->beacon_key, sizeof(meshx_key_t));
    /* nid, encryption key, privacy key */
    P[0] = 0x00;
    meshx_k2(pnet_key->net_key, P, 1, &pnet_key->nid, pnet_key->encryption_key,
             pnet_key->privacy_key);
    MESHX_DEBUG("nid: 0x%x", pnet_key->nid);
    MESHX_DEBUG("encryption key:");
    MESHX_DUMP_DEBUG(pnet_key->encryption_key, sizeof(meshx_key_t));
    MESHX_DEBUG("privacy key:");
    MESHX_DUMP_DEBUG(pnet_key->privacy_key, sizeof(meshx_key_t));
    /* network id */
    meshx_k3(pnet_key->net_key, pnet_key->network_id);
    MESHX_DEBUG("network id:");
    MESHX_DUMP_DEBUG(pnet_key->network_id, sizeof(meshx_network_id_t));
}

int32_t meshx_net_key_add(uint16_t net_key_index, meshx_key_t net_key)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            MESHX_ERROR("net key index alreay used: %d", net_key_index);
            return -MESHX_ERR_ALREADY;
        }
    }

    if (meshx_list_length(&meshx_net_keys) >= meshx_node_params.config.net_key_num)
    {
        MESHX_ERROR("net key num reaches maximum number: %d", meshx_node_params.config.net_key_num);
        return -MESHX_ERR_RESOURCE;
    }

    pnet_key = meshx_malloc(sizeof(meshx_net_key_info_t));
    if (NULL == pnet_key)
    {
        MESHX_ERROR("can't add net key: out of memory");
        return -MESHX_ERR_MEM;
    }

    pnet_key->net_key.net_key_index = net_key_index;
    memcpy(pnet_key->net_key.net_key, net_key, sizeof(meshx_key_t));
    MESHX_DEBUG("network key: index %d, value ", net_key_index);
    MESHX_DUMP_DEBUG(pnet_key->net_key.net_key, sizeof(meshx_key_t));
    meshx_net_key_derive(&pnet_key->net_key);
    meshx_list_append(&meshx_net_keys, &pnet_key->node);

    return MESHX_SUCCESS;
}

int32_t meshx_net_key_update(uint16_t net_key_index, meshx_key_t net_key)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            /* TODO:need compare or process app key? */
            memcpy(pnet_key->net_key.net_key, net_key, sizeof(meshx_key_t));
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
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
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