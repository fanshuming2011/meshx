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
    meshx_net_key_t net_key;
    meshx_list_t node;
} meshx_net_key_info_t;

typedef struct
{
    meshx_app_key_t app_key;
    meshx_list_t node;
} meshx_app_key_info_t;

typedef struct
{
    meshx_device_key_t dev_key;
    meshx_list_t node;
} meshx_dev_key_info_t;


static meshx_list_t meshx_net_keys;
static meshx_list_t meshx_app_keys;
static meshx_list_t meshx_dev_keys;


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

const meshx_app_key_t *meshx_app_key_get(uint16_t app_key_index)
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

void meshx_app_key_traverse_start(const meshx_app_key_t **ptraverse_key, uint8_t aid)
{
    *ptraverse_key = NULL;
    meshx_list_t *pnode;
    meshx_app_key_info_t *papp_key;
    meshx_list_foreach(pnode, &meshx_app_keys)
    {
        papp_key = MESHX_CONTAINER_OF(pnode, meshx_app_key_info_t, node);
        if (papp_key->app_key.aid == aid)
        {
            *ptraverse_key = &papp_key->app_key;
            break;
        }
    }
}

void meshx_app_key_traverse_continue(const meshx_app_key_t **ptraverse_key, uint8_t aid)
{
    const meshx_app_key_info_t *pinfo = (const meshx_app_key_info_t *)(*ptraverse_key);
    *ptraverse_key = NULL;
    meshx_list_t *pnode;
    meshx_app_key_info_t *papp_key;
    for (pnode = pinfo->node.pnext; pnode != &meshx_net_keys; pnode = pnode->pnext)
    {
        papp_key = MESHX_CONTAINER_OF(pnode, meshx_app_key_info_t, node);
        if (papp_key->app_key.aid == aid)
        {
            *ptraverse_key = &papp_key->app_key;
            break;
        }
    }
}

static void meshx_app_key_derive(meshx_app_key_t *papp_key)
{
    meshx_k4(papp_key->app_key, &papp_key->aid);
    MESHX_INFO("aid: 0x%x", papp_key->aid);
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
    MESHX_INFO("application key add: index %d-%d", app_key_index, net_key_index);
    MESHX_DUMP_INFO(papp_key->app_key.app_key, sizeof(meshx_key_t));
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

const meshx_net_key_t *meshx_net_key_get(uint16_t net_key_index)
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

void meshx_net_key_traverse_start(const meshx_net_key_t **ptraverse_key, uint8_t nid)
{
    *ptraverse_key = NULL;
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
        if (pnet_key->net_key.nid == nid)
        {
            *ptraverse_key = &pnet_key->net_key;
            break;
        }
    }
}

void meshx_net_key_traverse_continue(const meshx_net_key_t **ptraverse_key, uint8_t nid)
{
    const meshx_net_key_info_t *pinfo = (const meshx_net_key_info_t *)(*ptraverse_key);
    *ptraverse_key = NULL;
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key;
    for (pnode = pinfo->node.pnext; pnode != &meshx_net_keys; pnode = pnode->pnext)
    {
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
        if (pnet_key->net_key.nid == nid)
        {
            *ptraverse_key = &pnet_key->net_key;
            break;
        }
    }
}

static void meshx_net_key_derive(meshx_net_key_t *pnet_key)
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
    MESHX_INFO("nid: 0x%x", pnet_key->nid);
    MESHX_DEBUG("encryption key:");
    MESHX_DUMP_DEBUG(pnet_key->encryption_key, sizeof(meshx_key_t));
    MESHX_DEBUG("privacy key:");
    MESHX_DUMP_DEBUG(pnet_key->privacy_key, sizeof(meshx_key_t));
    /* network id */
    meshx_k3(pnet_key->net_key, pnet_key->net_id);
    MESHX_DEBUG("network id:");
    MESHX_DUMP_DEBUG(pnet_key->net_id, sizeof(meshx_net_id_t));
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
    MESHX_INFO("network key add: index %d", net_key_index);
    MESHX_DUMP_INFO(pnet_key->net_key.net_key, sizeof(meshx_key_t));
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

int32_t meshx_net_key_delete(uint16_t net_key_index)
{
    meshx_list_t *pnode;
    meshx_net_key_info_t *pnet_key = NULL;
    meshx_list_foreach(pnode, &meshx_net_keys)
    {
        pnet_key = MESHX_CONTAINER_OF(pnode, meshx_net_key_info_t, node);
        if (pnet_key->net_key.net_key_index == net_key_index)
        {
            meshx_list_remove(pnode);
            /* TODO: update all bind app keys? */
            meshx_free(pnet_key);
            return MESHX_SUCCESS;
        }
    }

    return -MESHX_ERR_NOT_FOUND;
}

void meshx_net_key_clear(void)
{
}

int32_t meshx_dev_key_init(void)
{
    meshx_list_init_head(&meshx_dev_keys);
    return MESHX_SUCCESS;
}

const meshx_device_key_t *meshx_dev_key_get(uint16_t addr)
{
    meshx_list_t *pnode;
    meshx_dev_key_info_t *pdev_key;
    meshx_list_foreach(pnode, &meshx_dev_keys)
    {
        pdev_key = MESHX_CONTAINER_OF(pnode, meshx_dev_key_info_t, node);
        if ((addr >= pdev_key->dev_key.primary_addr) &&
            (addr <= (pdev_key->dev_key.primary_addr + pdev_key->dev_key.element_num - 1)))
        {
            return &pdev_key->dev_key;
        }
    }

    return NULL;
}

int32_t meshx_dev_key_add(uint16_t primary_addr, uint8_t element_num, meshx_key_t dev_key)
{
    if ((MESHX_ADDRESS_UNASSIGNED == primary_addr) || (0 == element_num))
    {
        MESHX_ERROR("invalid parameters: primary addr 0x%04x, element num %d", primary_addr, element_num);
        return -MESHX_ERR_INVAL;
    }

    if (meshx_list_length(&meshx_dev_keys) >= meshx_node_params.config.dev_key_num)
    {
        MESHX_ERROR("dev key num reaches maximum number: %d", meshx_node_params.config.dev_key_num);
        return -MESHX_ERR_RESOURCE;
    }

    meshx_list_t *pnode;
    meshx_dev_key_info_t *pdev_key;
    uint16_t addr_start, addr_end;
    meshx_list_foreach(pnode, &meshx_dev_keys)
    {
        pdev_key = MESHX_CONTAINER_OF(pnode, meshx_dev_key_info_t, node);
        addr_start = pdev_key->dev_key.primary_addr;
        addr_end = pdev_key->dev_key.primary_addr + pdev_key->dev_key.element_num - 1;
        if (((primary_addr >= addr_start) && (primary_addr <= addr_end)) ||
            (((primary_addr + element_num - 1) >= addr_start) &&
             ((primary_addr + element_num - 1) <= addr_end)))
        {
            MESHX_ERROR("dev key addr range overlap with existed addr: add(0x%04x-%d), exist(0x%04x-%d)",
                        primary_addr, element_num, addr_start, pdev_key->dev_key.element_num);
            return -MESHX_ERR_ALREADY;
        }
    }

    pdev_key = meshx_malloc(sizeof(meshx_dev_key_info_t));
    if (NULL == pdev_key)
    {
        MESHX_ERROR("can't add dev key: out of memory");
        return -MESHX_ERR_MEM;
    }

    pdev_key->dev_key.primary_addr = primary_addr;
    pdev_key->dev_key.element_num = element_num;
    memcpy(pdev_key->dev_key.dev_key, dev_key, sizeof(meshx_key_t));
    MESHX_INFO("device key add: primary addr 0x%04x, element num %d", pdev_key->dev_key.primary_addr,
               pdev_key->dev_key.element_num);
    MESHX_DUMP_INFO(pdev_key->dev_key.dev_key, sizeof(meshx_key_t));
    meshx_list_append(&meshx_dev_keys, &pdev_key->node);

    return MESHX_SUCCESS;
}

void meshx_dev_key_delete(meshx_key_t dev_key)
{

}

void meshx_dev_key_delete_by_addr(uint16_t addr)
{

}

