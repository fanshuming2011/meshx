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
    meshx_bearer_t bearer = {.bearer = 0};
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

void meshx_bearer_delete(meshx_bearer_t bearer)
{
    switch (bearer.type)
    {
    case MESHX_BEARER_TYPE_ADV:
        meshx_bearer_adv_delete(bearer);
        break;
    case MESHX_BEARER_TYPE_GATT:
        meshx_bearer_gatt_delete(bearer);
        break;
    default:
        MESHX_WARN("meshx_bearer_delete: invalid bearer %d-%d", bearer.type, bearer.id);
        break;
    }
}

bool meshx_bearer_is_valid(meshx_bearer_t bearer)
{
    bool ret = FALSE;
    switch (bearer.type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_bearer_adv_is_valid(bearer);
        break;
    case MESHX_BEARER_TYPE_GATT:
        ret = meshx_bearer_gatt_is_valid(bearer);
        break;
    default:
        break;
    }

    return ret;
}

int32_t meshx_bearer_send(meshx_bearer_t bearer, meshx_bearer_pkt_type_t pkt_type,
                          const uint8_t *pdata, uint8_t len)
{
    int32_t ret = MESHX_SUCCESS;
    if (pkt_type >= MESHX_BEARER_PKT_TYPE_UNKONWN)
    {
        MESHX_ERROR("invalid packet type: %d", pkt_type);
        return -MESHX_ERR_INVAL;
    }
    switch (bearer.type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_bearer_adv_send(bearer, pkt_type, pdata, len);
        break;
    case MESHX_BEARER_TYPE_GATT:
        ret = meshx_bearer_gatt_send(bearer, pdata, len);
        break;
    default:
        MESHX_ERROR("invalid bearer: %d", bearer);
        ret = -MESHX_ERR_INVAL_BEARER;
        break;
    }

    return ret;
}

static meshx_bearer_t meshx_bearer_get(const meshx_bearer_rx_metadata_t *prx_metadata)
{
    MESHX_ASSERT(NULL != prx_metadata);
    meshx_bearer_t bearer = {.type = MESHX_BEARER_TYPE_INVALID, .id = 0};

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
    if (NULL == prx_metadata)
    {
        MESHX_WARN("can't handle NULL metadata");
        return -MESHX_ERR_INVAL;
    }

    meshx_bearer_t bearer = meshx_bearer_get(prx_metadata);
    if (MESHX_BEARER_TYPE_INVALID == bearer.type)
    {
        MESHX_WARN("invalid bearer: %d-%d", bearer.type, bearer.id);
        return -MESHX_ERR_INVAL_BEARER;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (bearer.type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_bearer_adv_receive(bearer, pdata, len);
        break;
    case MESHX_BEARER_TYPE_GATT:
        ret = meshx_bearer_gatt_receive(bearer, pdata, len);
        break;
    default:
        ret = -MESHX_ERR_INVAL;
        MESHX_WARN("can't handle bearer type(%d)", prx_metadata->bearer_type);
        break;
    }

    return ret;
}

