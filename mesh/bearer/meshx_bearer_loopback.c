/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_BEARER_ADV"
#include "meshx_trace.h"
#include "meshx_bearer.h"
#include "meshx_bearer_internal.h"
#include "meshx_assert.h"

struct _meshx_bearer meshx_loopback_bearer;

int32_t meshx_bearer_loopback_init(void)
{
    MESHX_INFO("initialize loopback bearer success");
    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_loopback_create(void)
{
    if (MESHX_BEARER_TYPE_ADV == meshx_loopback_bearer.type)
    {
        MESHX_WARN("bearer loopback already exists, only support one loopback bearer!");
        return NULL;
    }

    meshx_loopback_bearer.type = MESHX_BEARER_TYPE_LOOPBACK;
    meshx_loopback_bearer.net_iface = NULL;
    MESHX_INFO("create loopback bearer(0x%08x) success", &meshx_loopback_bearer);
    return &meshx_loopback_bearer;
}

void meshx_loopback_bearer_delete(meshx_bearer_t bearer)
{
    MESHX_ASSERT(NULL != bearer);
    memset(bearer, 0, sizeof(struct _meshx_bearer));
    MESHX_INFO("delete loopback bearer(0x%08x) success", bearer);
}

int32_t meshx_bearer_loopback_send(meshx_bearer_t bearer, uint8_t pkt_type,
                                   const uint8_t *pdata, uint8_t len)
{
    MESHX_ASSERT(NULL != bearer);
    MESHX_ASSERT(NULL != pdata);
    MESHX_ASSERT(MESHX_IS_BEARER_ADV_PKT_TYPE_VALID(pkt_type));


    if (MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG == pkt_type)             MESHX_GAP_ADTYPE_MESH_MSG
    {
        MESHX_ERROR("only mesh message can send on loopback bearer");
        return -MESHX_ERR_INVAL;
    }

    if (MESHX_BEARER_TYPE_LOOPBACK != bearer->type)
    {
        MESHX_ERROR("send failed: invalid bearer type(%d)", bearer->type);
        return -MESHX_ERR_INVAL;
    }

    return meshx_bearer_loopback_receive(bearer, pdata, len);
}

int32_t meshx_bearer_loopback_receive(meshx_bearer_t bearer, const uint8_t *pdata,
                                      uint8_t len)
{
    if (NULL == bearer->net_iface)
    {
        MESHX_WARN("loopback bearer(0x%08x) hasn't connected to any network interface!", bearer);
        return -MESHX_ERR_CONNECT;
    }

    return meshx_net_receive(bearer->net_iface, pdata, len);
}

meshx_bearer_t meshx_bearer_loopback_get(void)
{
    return (MESHX_BEARER_TYPE_INVALID == meshx_loopback_berer.type) ? NULL : &meshx_loopback_bearer;
}




