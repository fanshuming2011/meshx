/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MESHX_NETWORK_INTERFACE"
#include "meshx_trace.h"
#include "meshx_network.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_network_internal.h"

typedef struct
{
    bool valid;
    meshx_network_if_t network_if;
    meshx_bearer_t bearer;
    meshx_network_if_input_filter_t input_filter;
    meshx_network_if_output_filter_t output_filter;
    meshx_network_if_filter_info_t filter_info;
    meshx_list_t node;
} meshx_network_interface_t;

static meshx_list_t network_if_list;

static bool meshx_network_default_input_filter(meshx_network_if_t network_if,
                                               const meshx_network_if_input_metadata_t *pinput_metadata)
{
    return MESHX_SUCCESS;
}

static bool meshx_network_default_output_filter(meshx_network_if_t network_if,
                                                const meshx_network_if_output_metadata_t *pout_metadata)
{
    return MESHX_SUCCESS;
}

#if 0
static uint16_t meshx_request_network_if_id(void)
{
    if (meshx_list_is_empty(&network_if_list))
    {
        return 0;
    }

    meshx_list_t *pnode;
    meshx_network_interface_t *pinterface;
    meshx_list_foreach(pnode, &network_if_list)
    {
        pinterface = MESHX_CONTAINER_OF(pnode, meshx_network_interface_t, node);
    }

    return 0;
}
#endif

static meshx_network_interface_t *meshx_request_network_interface(void)
{
    meshx_list_t *pnode = NULL;
    meshx_network_interface_t *pinterface;

    meshx_list_foreach(pnode, &network_if_list)
    {
        pinterface = MESHX_CONTAINER_OF(pnode, meshx_network_interface_t, node);
        if (!pinterface->valid)
        {
            return pinterface;
        }
    }

    return meshx_malloc(sizeof(meshx_network_interface_t));
}

static void meshx_release_network_interface(meshx_network_interface_t *pinterface)
{
    pinterface->valid = FALSE;
}

static int32_t meshx_network_if_create_loopback(void)
{
    /* create loopback interface */
    meshx_network_interface_t *pinterface = meshx_request_network_interface();
    if (NULL == pinterface)
    {
        MESHX_ERROR("create network loopback interface failed: out of memeory");
        return -MESHX_ERR_NO_MEM;
    }
    memset(pinterface, 0, sizeof(meshx_network_interface_t));
    pinterface->valid = TRUE;
    pinterface->network_if.type = MESHX_NETWORK_IF_TYPE_LOOPBACK;
    meshx_list_append(&network_if_list, &pinterface->node);
    pinterface->network_if.id = meshx_list_index(&network_if_list, &pinterface->node);
    MESHX_INFO("create network loopback interface(%d) success", pinterface->network_if.network_if);
    return MESHX_SUCCESS;
}

int32_t meshx_network_if_init(void)
{
    meshx_list_init_head(&network_if_list);
    /* create loopback interface */
    return meshx_network_if_create_loopback();
}

meshx_network_if_t meshx_network_if_get(meshx_bearer_t bearer)
{
    meshx_list_t *pnode = NULL;
    meshx_network_interface_t *pinterface;
    meshx_network_if_t network_if = {.network_if = 0};

    meshx_list_foreach(pnode, &network_if_list)
    {
        pinterface = MESHX_CONTAINER_OF(pnode, meshx_network_interface_t, node);
        if (pinterface->bearer.bearer == bearer.bearer)
        {
            return pinterface->network_if;
        }
    }

    MESHX_WARN("bearer(%d) has not been connected to network interface!", bearer);
    return network_if;
}

meshx_bearer_t meshx_network_if_get_bearer(meshx_network_if_t network_if)
{
    meshx_list_t *pnode = NULL;
    meshx_network_interface_t *pinterface;
    meshx_bearer_t bearer = {.bearer = 0};

    meshx_list_foreach(pnode, &network_if_list)
    {
        pinterface = MESHX_CONTAINER_OF(pnode, meshx_network_interface_t, node);
        if (pinterface->network_if.network_if == network_if.network_if)
        {
            return pinterface->bearer;
        }
    }

    MESHX_WARN("network interface(%d) has not been connected to bearer!", network_if);
    return bearer;
}

static meshx_network_interface_t *meshx_network_if_get_by_bearer(meshx_bearer_t bearer)
{
    meshx_list_t *pnode = NULL;
    meshx_network_interface_t *pinterface;

    meshx_list_foreach(pnode, &network_if_list)
    {
        pinterface = MESHX_CONTAINER_OF(pnode, meshx_network_interface_t, node);
        if (pinterface->bearer.bearer == bearer.bearer)
        {
            return pinterface;
        }
    }

    MESHX_WARN("can not find network interface(%d)", bearer.bearer);
    return NULL;
}

static meshx_network_interface_t *meshx_network_if_get_by_if(meshx_network_if_t network_if)
{
    meshx_list_t *pnode = NULL;
    meshx_network_interface_t *pinterface;

    meshx_list_foreach(pnode, &network_if_list)
    {
        pinterface = MESHX_CONTAINER_OF(pnode, meshx_network_interface_t, node);
        if (pinterface->network_if.network_if == network_if.network_if)
        {
            return pinterface;
        }
    }

    MESHX_WARN("can not find network interface(%d)", network_if.network_if);
    return NULL;
}

meshx_network_if_t meshx_network_if_create(uint16_t type)
{
    meshx_network_if_t network_if = {.network_if = 0};
    /* create new interface */
    meshx_network_interface_t *pinterface = meshx_request_network_interface();
    if (NULL == pinterface)
    {
        MESHX_ERROR("create network interface failed: out of memory");
        return network_if;
    }
    memset(pinterface, 0, sizeof(meshx_network_interface_t));
    pinterface->valid = TRUE;
    meshx_list_append(&network_if_list, &pinterface->node);
    pinterface->network_if.type = type;
    pinterface->network_if.id = meshx_list_index(&network_if_list, &pinterface->node);

    MESHX_INFO("create network interface(%d) success", pinterface->network_if.network_if);
    return pinterface->network_if;
}

void meshx_network_if_delete(meshx_network_if_t network_if)
{
    meshx_list_t *pnode = NULL;
    meshx_network_interface_t *pinterface;

    meshx_list_foreach(pnode, &network_if_list)
    {
        pinterface = MESHX_CONTAINER_OF(pnode, meshx_network_interface_t, node);
        if (pinterface->network_if.network_if == network_if.network_if);
        {
            meshx_release_network_interface(pinterface);
            MESHX_INFO("delete network interface(%d) success", network_if.network_if);
            return ;
        }
    }

    MESHX_WARN("delete network interface(%d) failed: not found", network_if.network_if);
}

int32_t meshx_network_if_connect(meshx_network_if_t network_if, meshx_bearer_t bearer,
                                 meshx_network_if_input_filter_t in_filter, meshx_network_if_output_filter_t out_filter)
{
    /* check network interface and bearer type */
    if (((network_if.type == MESHX_NETWORK_IF_TYPE_ADV) && (bearer.type != MESHX_BEARER_TYPE_ADV)) ||
        ((network_if.type == MESHX_NETWORK_IF_TYPE_ADV) && (bearer.type != MESHX_BEARER_TYPE_ADV)))
    {
        MESHX_ERROR("network interface type(%d) does not match bearer type(%d)", network_if.type,
                    bearer.type);
        return -MESHX_ERR_INVAL;
    }

    /* find exists */
    meshx_list_t *pnode = NULL;
    meshx_network_interface_t *pinterface;
    meshx_list_foreach(pnode, &network_if_list)
    {
        pinterface = MESHX_CONTAINER_OF(pnode, meshx_network_interface_t, node);
        if (pinterface->bearer.bearer == bearer.bearer)
        {
            MESHX_WARN("bearer(%d) has already been connected to network interface(%d)!", bearer,
                       pinterface->network_if);
            return -MESHX_ERR_ALREADY;
        }
    }

    /* validation bearer */
    if (!meshx_bearer_is_valid(bearer))
    {
        MESHX_ERROR("connect bearer(%d) failed: invalid bearer", bearer);
        return -MESHX_ERR_INVAL_BEARER;
    }

    /* validation network interface */
    pinterface = meshx_network_if_get_by_if(network_if);
    if (NULL == pinterface)
    {
        MESHX_ERROR("connect bearer(%d) failed: invalid network infterface", bearer);
        return -MESHX_ERR_INVAL_NETWORK_IF;
    }

    if (MESHX_BEARER_TYPE_ADV == bearer.type)
    {
        pinterface->network_if.type = MESHX_NETWORK_IF_TYPE_ADV;
    }
    else
    {
        pinterface->network_if.type = MESHX_NETWORK_IF_TYPE_GATT;
    }

    pinterface->bearer = bearer;
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

    MESHX_INFO("connect bearer(%d) to network interface(%d) success", pinterface->bearer,
               pinterface->network_if);
    return MESHX_SUCCESS;
}

static void meshx_network_if_disconnect_intern(meshx_network_interface_t *pinterface)
{
    pinterface->bearer.bearer = 0;
    memset(&pinterface->filter_info, 0, sizeof(meshx_network_if_filter_info_t));
    pinterface->input_filter = NULL;
    pinterface->output_filter = NULL;
}

void meshx_network_if_disconnect(meshx_network_if_t network_if)
{
    /* validation network interface */
    meshx_network_interface_t *pinterface = meshx_network_if_get_by_if(network_if);
    if (NULL == pinterface)
    {
        MESHX_ERROR("disconnect(%d) failed: invalid network interface", network_if.network_if);
        return ;
    }
    meshx_network_if_disconnect_intern(pinterface);
}

void meshx_network_if_disconnect_bearer(meshx_bearer_t bearer)
{
    /* validation bearer */
    if (!meshx_bearer_is_valid(bearer))
    {
        MESHX_ERROR("disconnect(%d) failed: invalid bearer", bearer.bearer);
        return ;
    }

    meshx_network_interface_t *pinterface = meshx_network_if_get_by_bearer(bearer);
    MESHX_ASSERT(NULL != pinterface);
    meshx_network_if_disconnect_intern(pinterface);
}

bool meshx_network_if_input_filter(meshx_network_if_t network_if, const meshx_network_pdu_t *ppdu,
                                   uint8_t pdu_len)
{
    meshx_network_interface_t *pinterface = meshx_network_if_get_by_if(network_if);
    MESHX_ASSERT(NULL != pinterface);
    meshx_network_if_input_metadata_t input_metadata;
    memset(&input_metadata, 0, sizeof(meshx_network_if_input_metadata_t));
    pinterface->filter_info.total_receive ++;
    if (!pinterface->input_filter(pinterface->network_if, &input_metadata))
    {
        pinterface->filter_info.filtered_receive ++;
        return FALSE;
    }

    return TRUE;
}

bool meshx_network_if_output_filter(meshx_network_if_t network_if, const meshx_network_pdu_t *ppdu,
                                    uint8_t pdu_len)
{
    meshx_network_interface_t *pinterface = meshx_network_if_get_by_if(network_if);
    MESHX_ASSERT(NULL != pinterface);

    meshx_network_if_output_metadata_t output_metadata;
    output_metadata.src_addr = ppdu->net_metadata.src;
    output_metadata.dst_addr = ppdu->net_metadata.dst;
    pinterface->filter_info.total_send ++;
    if (!pinterface->output_filter(pinterface->network_if, &output_metadata))
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
    meshx_network_interface_t *pinterface = meshx_network_if_get_by_if(network_if);
    if (NULL != pinterface)
    {
        filter_info = pinterface->filter_info;
    }

    return filter_info;
}