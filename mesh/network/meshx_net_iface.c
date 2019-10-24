/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_NET_IFACE"
#include "meshx_trace.h"
#include "meshx_net.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_net_internal.h"
#include "meshx_bearer_internal.h"
#include "meshx_list.h"

typedef struct
{
    uint8_t type;
    meshx_bearer_t bearer;
    meshx_net_iface_ifilter_t ifilter;
    meshx_net_iface_ofilter_t ofilter;
    meshx_net_iface_filter_info_t filter_info;
    meshx_list_t node;
} meshx_net_iface_info_t;

static meshx_list_t net_iface_list;

static bool meshx_net_default_ifilter(meshx_net_iface_t net_iface,
                                      const meshx_net_iface_ifilter_data_t *pdata)
{
    MESHX_DEBUG("input filter");
    return TRUE;
}

static bool meshx_net_default_ofilter(meshx_net_iface_t net_iface,
                                      const meshx_net_iface_ofilter_data_t *pdata)
{
    MESHX_DEBUG("output filter: src 0x%04x, dst 0x%04x", pdata->src_addr, pdata->dst_addr);
    return TRUE;
}

static meshx_net_iface_info_t *meshx_request_net_iface(void)
{
    if (meshx_list_length(&net_iface_list) >= MESHX_NET_IFACE_MAX_NUM)
    {
        MESHX_ERROR("net interface reached maximum number: %d", MESHX_NET_IFACE_MAX_NUM);
        return NULL;
    }

    return meshx_malloc(sizeof(meshx_net_iface_info_t));
}

static void meshx_release_net_iface(meshx_net_iface_info_t *piface)
{
    meshx_list_remove(&piface->node);
    if (NULL != piface->bearer)
    {
        piface->bearer->net_iface = NULL;
    }
    meshx_free(piface);
}

static int32_t meshx_net_iface_create_loopback(void)
{
    /* create loopback interface */
    meshx_net_iface_info_t *piface = meshx_request_net_iface();
    if (NULL == piface)
    {
        MESHX_ERROR("create net loopback interface failed: out of memeory");
        return -MESHX_ERR_MEM;
    }
    memset(piface, 0, sizeof(meshx_net_iface_info_t));
    piface->type = MESHX_NET_IFACE_TYPE_LOOPBACK;
    piface->ifilter = meshx_net_default_ifilter;
    piface->ofilter = meshx_net_default_ofilter;
    meshx_list_append(&net_iface_list, &piface->node);
    MESHX_INFO("create net loopback interface(0x%08x) success", piface);
    return MESHX_SUCCESS;
}

int32_t meshx_net_iface_init(void)
{
    meshx_list_init_head(&net_iface_list);
    /* create loopback interface */
    return meshx_net_iface_create_loopback();
}

meshx_net_iface_t meshx_net_iface_create(void)
{
    /* create new interface */
    meshx_net_iface_info_t *piface = meshx_request_net_iface();
    if (NULL == piface)
    {
        MESHX_ERROR("create net interface failed");
        return NULL;
    }
    memset(piface, 0, sizeof(meshx_net_iface_t));
    meshx_list_append(&net_iface_list, &piface->node);

    MESHX_INFO("create net interface(0x%08x) success", piface);
    return piface;
}

void meshx_net_iface_delete(meshx_net_iface_t net_iface)
{
    if (NULL != net_iface)
    {
        meshx_net_iface_info_t *piface = (meshx_net_iface_info_t *)net_iface;
        meshx_release_net_iface(piface);
        MESHX_INFO("delete net interface(0x%08x) success", net_iface);
    }
}

int32_t meshx_net_iface_connect(meshx_net_iface_t net_iface, meshx_bearer_t bearer,
                                meshx_net_iface_ifilter_t in_filter, meshx_net_iface_ofilter_t out_filter)
{
    if ((NULL == net_iface) || (NULL == bearer))
    {
        MESHX_ERROR("invalid net interface or bearer: 0x%08x-0x%08x", net_iface, bearer);
        return -MESHX_ERR_INVAL;
    }

    meshx_net_iface_info_t *piface = (meshx_net_iface_info_t *)net_iface;
    if (NULL != piface->bearer)
    {
        MESHX_WARN("net interface(0x%08x) has already been connected to bearer(0x%08x)!", piface,
                   piface->bearer);
        return -MESHX_ERR_ALREADY;
    }

    if (NULL != bearer->net_iface)
    {
        MESHX_WARN("bearer(0x%08x) has already been connected to net interface(0x%08x)!", bearer,
                   bearer->net_iface);

        return -MESHX_ERR_ALREADY;
    }

    if (MESHX_BEARER_TYPE_ADV == bearer->type)
    {
        piface->type = MESHX_NET_IFACE_TYPE_ADV;
    }
    else
    {
        piface->type = MESHX_NET_IFACE_TYPE_GATT;
    }

    piface->bearer = bearer;
    bearer->net_iface = piface;
    if (NULL == in_filter)
    {
        piface->ifilter = meshx_net_default_ifilter;
    }
    else
    {
        piface->ifilter = in_filter;
    }

    if (NULL == out_filter)
    {
        piface->ofilter = meshx_net_default_ofilter;;
    }
    else
    {
        piface->ofilter = out_filter;
    }

    MESHX_INFO("connect bearer(0x%08x) to net interface(0x%08x) success", bearer,
               piface);
    return MESHX_SUCCESS;
}

void meshx_net_iface_disconnect(meshx_net_iface_t net_iface)
{
    if (NULL == net_iface)
    {
        MESHX_ERROR("invalid net interface: 0x%08x", net_iface);
        return ;
    }

    meshx_net_iface_info_t *piface = (meshx_net_iface_info_t *)net_iface;
    piface->type = MESHX_NET_IFACE_TYPE_INVALID;
    if (NULL != piface->bearer)
    {
        piface->bearer->net_iface = NULL;
    }
    piface->bearer = NULL;
    memset(&piface->filter_info, 0, sizeof(meshx_net_iface_filter_info_t));
    piface->ifilter = NULL;
    piface->ofilter = NULL;
}

bool meshx_net_iface_is_connect(meshx_net_iface_t net_iface)
{
    if (NULL == net_iface)
    {
        MESHX_ERROR("invalid net interface: 0x%08x", net_iface);
        return FALSE;
    }

    meshx_net_iface_info_t *piface = (meshx_net_iface_info_t *)net_iface;
    if (MESHX_NET_IFACE_TYPE_INVALID == piface->type)
    {
        return FALSE;
    }

    if (MESHX_NET_IFACE_TYPE_LOOPBACK == piface->type)
    {
        return TRUE;
    }

    return (NULL != piface->bearer);
}

bool meshx_net_iface_ifilter(meshx_net_iface_t net_iface,
                             const meshx_net_iface_ifilter_data_t *pdata)
{
    MESHX_ASSERT(NULL != net_iface);
    meshx_net_iface_info_t *piface = (meshx_net_iface_info_t *)net_iface;

    piface->filter_info.total_receive ++;
    if (!piface->ifilter(piface, pdata))
    {
        piface->filter_info.filtered_receive ++;
        return FALSE;
    }

    return TRUE;
}

bool meshx_net_iface_ofilter(meshx_net_iface_t net_iface,
                             const meshx_net_iface_ofilter_data_t *pdata)
{
    MESHX_ASSERT(NULL != net_iface);
    meshx_net_iface_info_t *piface = (meshx_net_iface_info_t *)net_iface;

    piface->filter_info.total_send ++;
    if (!piface->ofilter(piface, pdata))
    {
        piface->filter_info.filtered_send ++;
        return FALSE;
    }

    return TRUE;
}

meshx_net_iface_filter_info_t meshx_net_iface_get_filter_info(meshx_net_iface_t net_iface)
{
    meshx_net_iface_filter_info_t filter_info;
    memset(&filter_info, 0, sizeof(meshx_net_iface_filter_info_t));
    meshx_net_iface_info_t *piface = (meshx_net_iface_info_t *)net_iface;
    if (NULL == piface)
    {
        return filter_info;
    }

    return piface->filter_info;
}

meshx_net_iface_t meshx_net_iface_get(meshx_bearer_t bearer)
{
    if (NULL == bearer)
    {
        return NULL;
    }

    return bearer->net_iface;
}

uint8_t meshx_net_iface_type_get(meshx_net_iface_t net_iface)
{
    if (NULL == net_iface)
    {
        return MESHX_NET_IFACE_TYPE_INVALID;
    }

    return ((meshx_net_iface_info_t *)net_iface)->type;

}

meshx_bearer_t meshx_net_iface_get_bearer(meshx_net_iface_t net_iface)
{
    if (NULL == net_iface)
    {
        return NULL;
    }

    return ((meshx_net_iface_info_t *)net_iface)->bearer;
}