/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_BEARER"
#include "meshx_bearer.h"
#include "meshx_bearer_internal.h"
#include "meshx_errno.h"
#include "meshx_trace.h"
#include "meshx_assert.h"
#include "meshx_gap.h"
#include "meshx_network.h"

int32_t meshx_bearer_init(void)
{
    meshx_bearer_adv_init();
    meshx_bearer_gatt_init();

    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_create(meshx_bearer_param_t bearer_param)
{
    meshx_bearer_t bearer = NULL;
    switch (bearer_param.bearer_type)
    {
    case MESHX_BEARER_TYPE_ADV:
        bearer = meshx_bearer_adv_create(bearer_param.param_adv);
        break;
    case MESHX_BEARER_TYPE_GATT:
        bearer = meshx_bearer_gatt_create(bearer_param.param_gatt);
        break;
    default:
        MESHX_ERROR("create failed: invalid bearer type %d", bearer_param.bearer_type);
        break;
    }

    return bearer;
}

uint8_t meshx_bearer_type_get(meshx_bearer_t bearer)
{
    return (NULL == bearer) ? MESHX_BEARER_TYPE_INVALID : bearer->type;
}

void meshx_bearer_delete(meshx_bearer_t bearer)
{
    if (NULL != bearer)
    {
        switch (bearer->type)
        {
        case MESHX_BEARER_TYPE_ADV:
            meshx_bearer_adv_delete(bearer);
            break;
        case MESHX_BEARER_TYPE_GATT:
            meshx_bearer_gatt_delete(bearer);
            break;
        default:
            MESHX_WARN("invalid bearer type: %d", bearer->type);
            break;
        }
    }
}

int32_t meshx_bearer_send(meshx_bearer_t bearer, uint8_t pkt_type,
                          const uint8_t *pdata, uint8_t len)
{
    if ((NULL == bearer) || (NULL == pdata))
    {
        MESHX_ERROR("bearer or data is NULL: 0x%x-0x%x", bearer, pdata);
        return MESHX_ERR_INVAL;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_bearer_adv_send(bearer, pkt_type, pdata, len);
        break;
    case MESHX_BEARER_TYPE_GATT:
        ret = meshx_bearer_gatt_send(bearer, pkt_type, pdata, len);
        break;
    default:
        MESHX_ERROR("invalid bearer type: %d", bearer->type);
        ret = MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

static meshx_bearer_t meshx_bearer_get(const meshx_bearer_rx_metadata_t *prx_metadata)
{
    if (NULL == prx_metadata)
    {
        MESHX_WARN("rx metadta is NULL");
        return NULL;
    }
    meshx_bearer_t bearer = NULL;

    switch (prx_metadata->bearer_type)
    {
    case MESHX_BEARER_TYPE_ADV:
        bearer = meshx_bearer_adv_get();
        break;
    case MESHX_BEARER_TYPE_GATT:
        bearer = meshx_bearer_gatt_get(&prx_metadata->gatt_metadata);
        break;
    default:
        MESHX_WARN("can't handle receive type(%d)", prx_metadata->bearer_type);
        break;
    }

    return bearer;
}

int32_t meshx_bearer_receive(const uint8_t *pdata, uint8_t len,
                             const meshx_bearer_rx_metadata_t *prx_metadata)
{
    if ((NULL == prx_metadata) || ((NULL == pdata) && (0 != len)))
    {
        MESHX_WARN("can't handle NULL metadata or data: 0x%08x-0x%08x-%d", prx_metadata, pdata, len);
        return MESHX_ERR_INVAL;
    }

    meshx_bearer_t bearer = meshx_bearer_get(prx_metadata);
    if (NULL == bearer)
    {
        MESHX_WARN("no bearer can handle received data");
        return MESHX_ERR_RESOURCE;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_bearer_adv_receive(bearer, MESHX_GAP_GET_ADV_TYPE(pdata), MESHX_GAP_GET_ADV_PDU(pdata),
                                       MESHX_GAP_GET_ADV_PDU_LEN(pdata));
        break;
    case MESHX_BEARER_TYPE_GATT:
        ret = meshx_bearer_gatt_receive(bearer, pdata, len);
        break;
    default:
        ret = MESHX_ERR_INVAL;
        MESHX_WARN("can't handle received bearer type: %d", prx_metadata->bearer_type);
        break;
    }

    return ret;
}

