/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_NETWORK_IF"
#include "meshx_trace.h"
#include "meshx_network.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_network_internal.h"
#include "meshx_bearer_internal.h"
#include "meshx_list.h"

typedef struct
{
    uint8_t type;
    meshx_bearer_t bearer;
    meshx_network_if_input_filter_t input_filter;
    meshx_network_if_output_filter_t output_filter;
    meshx_network_if_filter_info_t filter_info;
    meshx_list_t node;
} meshx_network_interface_t;

static meshx_list_t network_if_list;

static bool meshx_network_default_input_filter(meshx_network_if_t network_if,
                                               const meshx_network_if_input_filter_data_t *pdata)
{
    MESHX_DEBUG("input filter");
    return TRUE;
}

static bool meshx_network_default_output_filter(meshx_network_if_t network_if,
                                                const meshx_network_if_output_filter_data_t *pdata)
{
    MESHX_DEBUG("output filter: src 0x%04x, dst 0x%04x", pdata->src_addr, pdata->dst_addr);
    return TRUE;
}

static meshx_network_interface_t *meshx_request_network_interface(void)
{
    if (meshx_list_length(&network_if_list) >= MESHX_NETWORK_IF_MAX_NUM)
    {
        MESHX_ERROR("network interface reached maximum number: %d", MESHX_NETWORK_IF_MAX_NUM);
        return NULL;
    }

    return meshx_malloc(sizeof(meshx_network_interface_t));
}

static void meshx_release_network_interface(meshx_network_interface_t *pinterface)
{
    meshx_list_remove(&pinterface->node);
    if (NULL != pinterface->bearer)
    {
        pinterface->bearer->network_if = NULL;
    }
    meshx_free(pinterface);
}

static int32_t meshx_network_if_create_loopback(void)
{
    /* create loopback interface */
    meshx_network_interface_t *pinterface = meshx_request_network_interface();
    if (NULL == pinterface)
    {
        MESHX_ERROR("create network loopback interface failed: out of memeory");
        return -MESHX_ERR_MEM;
    }
    memset(pinterface, 0, sizeof(meshx_network_interface_t));
    pinterface->type = MESHX_NETWORK_IF_TYPE_LOOPBACK;
    pinterface->input_filter = meshx_network_default_input_filter;
    pinterface->output_filter = meshx_network_default_output_filter;
    meshx_list_append(&network_if_list, &pinterface->node);
    MESHX_INFO("create network loopback interface(0x%08x) success", pinterface);
    return MESHX_SUCCESS;
}

int32_t meshx_network_if_init(void)
{
    meshx_list_init_head(&network_if_list);
    /* create loopback interface */
    return meshx_network_if_create_loopback();
}

meshx_network_if_t meshx_network_if_create(void)
{
    /* create new interface */
    meshx_network_interface_t *pinterface = meshx_request_network_interface();
    if (NULL == pinterface)
    {
        MESHX_ERROR("create network interface failed");
        return NULL;
    }
    memset(pinterface, 0, sizeof(meshx_network_interface_t));
    meshx_list_append(&network_if_list, &pinterface->node);

    MESHX_INFO("create network interface(0x%08x) success", pinterface);
    return pinterface;
}

void meshx_network_if_delete(meshx_network_if_t network_if)
{
    if (NULL != network_if)
    {
        meshx_network_interface_t *pinterface = (meshx_network_interface_t *)network_if;
        meshx_release_network_interface(pinterface);
        MESHX_INFO("delete network interface(0x%08x) success", network_if);
    }
}

int32_t meshx_network_if_connect(meshx_network_if_t network_if, meshx_bearer_t bearer,
                                 meshx_network_if_input_filter_t in_filter, meshx_network_if_output_filter_t out_filter)
{
    if ((NULL == network_if) || (NULL == bearer))
    {
        MESHX_ERROR("invalid network interface or bearer: 0x%08x-0x%08x", network_if, bearer);
        return -MESHX_ERR_INVAL;
    }

    meshx_network_interface_t *pinterface = (meshx_network_interface_t *)network_if;
    if (NULL != pinterface->bearer)
    {
        MESHX_WARN("network interface(0x%08x) has already been connected to bearer(0x%08x)!", pinterface,
                   pinterface->bearer);
        return -MESHX_ERR_ALREADY;
    }

    if (NULL != bearer->network_if)
    {
        MESHX_WARN("bearer(0x%08x) has already been connected to network interface(0x%08x)!", bearer,
                   bearer->network_if);

        return -MESHX_ERR_ALREADY;
    }

    if (MESHX_BEARER_TYPE_ADV == bearer->type)
    {
        pinterface->type = MESHX_NETWORK_IF_TYPE_ADV;
    }
    else
    {
        pinterface->type = MESHX_NETWORK_IF_TYPE_GATT;
    }

    pinterface->bearer = bearer;
    bearer->network_if = pinterface;
    if (NULL == in_filter)
    {
        pinterface->input_filter = meshx_network_default_input_filter;
    }
    else
    {
        pinterface->input_filter = in_filter;
    }

    if (NULL == out_filter)
    {
        pinterface->output_filter = meshx_network_default_output_filter;;
    }
    else
    {
        pinterface->output_filter = out_filter;
    }

    MESHX_INFO("connect bearer(0x%08x) to network interface(0x%08x) success", bearer,
               pinterface);
    return MESHX_SUCCESS;
}

void meshx_network_if_disconnect(meshx_network_if_t network_if)
{
    if (NULL == network_if)
    {
        MESHX_ERROR("invalid network interface: 0x%08x", network_if);
        return ;
    }

    meshx_network_interface_t *pinterface = (meshx_network_interface_t *)network_if;
    pinterface->type = MESHX_NETWORK_IF_TYPE_INVALID;
    if (NULL != pinterface->bearer)
    {
        pinterface->bearer->network_if = NULL;
    }
    pinterface->bearer = NULL;
    memset(&pinterface->filter_info, 0, sizeof(meshx_network_if_filter_info_t));
    pinterface->input_filter = NULL;
    pinterface->output_filter = NULL;
}

bool meshx_network_if_input_filter(meshx_network_if_t network_if,
                                   const meshx_network_if_input_filter_data_t *pdata)
{
    MESHX_ASSERT(NULL != network_if);
    meshx_network_interface_t *pinterface = (meshx_network_interface_t *)network_if;

    pinterface->filter_info.total_receive ++;
    if (!pinterface->input_filter(pinterface, pdata))
    {
        pinterface->filter_info.filtered_receive ++;
        return FALSE;
    }

    return TRUE;
}

bool meshx_network_if_output_filter(meshx_network_if_t network_if,
                                    const meshx_network_if_output_filter_data_t *pdata)
{
    MESHX_ASSERT(NULL != network_if);
    meshx_network_interface_t *pinterface = (meshx_network_interface_t *)network_if;

    pinterface->filter_info.total_send ++;
    if (!pinterface->output_filter(pinterface, pdata))
    {
        pinterface->filter_info.filtered_send ++;
        return FALSE;
    }

    return TRUE;
}

meshx_network_if_filter_info_t meshx_network_if_get_filter_info(meshx_network_if_t network_if)
{
    meshx_network_if_filter_info_t filter_info;
    memset(&filter_info, 0, sizeof(meshx_network_if_filter_info_t));
    meshx_network_interface_t *pinterface = (meshx_network_interface_t *)network_if;
    if (NULL == pinterface)
    {
        return filter_info;
    }

    return pinterface->filter_info;
}

meshx_network_if_t meshx_network_if_get(meshx_bearer_t bearer)
{
    if (NULL == bearer)
    {
        return NULL;
    }

    return bearer->network_if;
}

meshx_bearer_t meshx_network_if_get_bearer(meshx_network_if_t network_if)
{
    if (NULL == network_if)
    {
        return NULL;
    }

    return ((meshx_network_interface_t *)network_if)->bearer;
}