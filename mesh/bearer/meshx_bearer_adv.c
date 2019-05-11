/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MESHX_BEARER_ADV"
#include "meshx_trace.h"
#include "meshx_bearer.h"
#include "meshx_bearer_internal.h"
#include "meshx_errno.h"
#include "meshx_gap.h"
#include "meshx_assert.h"
#include "meshx_list.h"
#include "meshx_mem.h"
#include "meshx_network.h"
#include "meshx_provision.h"

typedef union
{
    struct
    {
        uint8_t length;
        uint8_t ad_type;
        uint8_t pdu[MESHX_GAP_ADV_DATA_MAX_LEN - 2];
    };

    uint8_t buffer[MESHX_GAP_ADV_DATA_MAX_LEN];
} meshx_bearer_adv_pkt_t;

typedef struct
{
    meshx_bearer_t bearer;
    meshx_bearer_param_adv_t param;
} meshx_bearer_adv_t;

#define MESHX_BEARER_ADV_ID       0
static meshx_bearer_adv_t bearer_adv;

int32_t meshx_bearer_adv_init(void)
{
    MESHX_INFO("initialize adv bearer success");
    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_adv_create(meshx_bearer_param_adv_t adv_param)
{
    if (MESHX_BEARER_TYPE_ADV == bearer_adv.bearer.type)
    {
        MESHX_WARN("bearer adv already exists, delete it first!");
        meshx_bearer_t bearer = {.bearer = 0};
        return bearer;
    }

    bearer_adv.bearer.type = MESHX_BEARER_TYPE_ADV;
    bearer_adv.bearer.id = MESHX_BEARER_ADV_ID;
    bearer_adv.param = adv_param;
    MESHX_INFO("create adv bearer(%d) success", bearer_adv.bearer.bearer);
    return bearer_adv.bearer;
}

void meshx_bearer_adv_delete(meshx_bearer_t bearer)
{
    if (bearer.bearer == bearer_adv.bearer.bearer)
    {
        MESHX_INFO("deinit adv bearer(%d) success", bearer_adv.bearer.bearer);
        bearer_adv.bearer.bearer = 0;
    }
}

int32_t meshx_bearer_adv_send(meshx_bearer_t bearer, meshx_bearer_pkt_type_t pkt_type,
                              const uint8_t *pdata, uint8_t len)
{
    MESHX_ASSERT(pkt_type < MESHX_BEARER_PKT_TYPE_UNKONWN);

    if ((bearer.bearer != bearer_adv.bearer.bearer) ||
        (MESHX_BEARER_TYPE_ADV != bearer.type) ||
        (MESHX_BEARER_ADV_ID != bearer.id))

    {
        MESHX_WARN("send failed: adv bearer(%d) is invalid", bearer.bearer);
        return -MESHX_ERR_INVAL_BEARER;
    }

    if (len > MESHX_GAP_ADV_DATA_MAX_LEN - 2)
    {
        MESHX_ERROR("invalid length: %d", len);
        return -MESHX_ERR_LENGTH;
    }
    meshx_bearer_adv_pkt_t adv_data;
    adv_data.length = len + 1;
    if (MESHX_BEARER_PKT_TYPE_PB_ADV == pkt_type)
    {
        adv_data.ad_type = MESHX_GAP_ADTYPE_PB_ADV;
    }
    else if (MESHX_BEARER_PKT_TYPE_MESH_MSG == pkt_type)
    {
        adv_data.ad_type = MESHX_GAP_ADTYPE_MESH_MSG;
    }
    else
    {
        adv_data.ad_type = MESHX_GAP_ADTYPE_MESH_BEACON;
    }
    memcpy(adv_data.pdu, pdata, len);

    MESHX_INFO("send adv data:  ");
    MESHX_DUMP_INFO(adv_data.buffer, len + 2);

    meshx_gap_action_t action;
    action.action_type = MESHX_GAP_ACTION_TYPE_ADV;
    action.action_adv_data.adv_type = MESHX_GAP_ADV_TYPE_NONCONN_IND;
    memcpy(action.action_adv_data.data, adv_data.buffer, len + 2);
    action.action_adv_data.data_len = len + 2;
    action.action_adv_data.period = bearer_adv.param.adv_period;

    return meshx_gap_add_action(&action);
}

int32_t meshx_bearer_adv_receive(meshx_bearer_t bearer, uint8_t adv_type, const uint8_t *pdata,
                                 uint8_t len)
{
    int32_t ret = MESHX_SUCCESS;
    switch (adv_type)
    {
    case MESHX_GAP_ADTYPE_MESH_MSG:
        MESHX_INFO("receive mesh msg");
        ret = meshx_network_receive(bearer, pdata, len);
        break;
    case MESHX_GAP_ADTYPE_PB_ADV:
        MESHX_INFO("receive pb-adv msg");
        ret = meshx_provision_receive(bearer, pdata, len);
        break;
    case MESHX_GAP_ADTYPE_MESH_BEACON:
        MESHX_INFO("receive beacon msg");
        break;
    default:
        MESHX_DEBUG("received no mesh message: 0x%x", adv_type);
        ret = -MESHX_ERR_INVAL_ADTYPE;
        break;
    }

    return ret;
}

meshx_bearer_t meshx_bearer_adv_get(void)
{
    return bearer_adv.bearer;
}

bool meshx_bearer_adv_is_valid(meshx_bearer_t bearer)
{
    return ((MESHX_BEARER_TYPE_ADV == bearer_adv.bearer.type) &&
            (MESHX_BEARER_ADV_ID == bearer_adv.bearer.id));
}
