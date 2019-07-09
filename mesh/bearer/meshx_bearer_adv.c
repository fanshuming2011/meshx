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
#include "meshx_errno.h"
#include "meshx_gap.h"
#include "meshx_assert.h"
#include "meshx_list.h"
#include "meshx_mem.h"
#include "meshx_network.h"
#include "meshx_provision_internal.h"
#include "meshx_beacon_internal.h"

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
    struct _meshx_bearer bearer;
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
        MESHX_WARN("bearer adv already exists, only support one adv bearer!");
        return NULL;
    }

    bearer_adv.bearer.type = MESHX_BEARER_TYPE_ADV;
    bearer_adv.bearer.network_if = NULL;
    bearer_adv.param = adv_param;
    MESHX_INFO("create adv bearer(0x%08x) success", &bearer_adv);
    return &bearer_adv.bearer;
}

void meshx_bearer_adv_delete(meshx_bearer_t bearer)
{
    MESHX_ASSERT(NULL != bearer);
    memset(bearer, 0, sizeof(meshx_bearer_adv_t));
    MESHX_INFO("delete adv bearer(0x%08x) success", bearer);
}

int32_t meshx_bearer_adv_send(meshx_bearer_t bearer, uint8_t pkt_type,
                              const uint8_t *pdata, uint8_t len, uint32_t repeat_times)
{
    MESHX_ASSERT(NULL != bearer);
    MESHX_ASSERT(NULL != pdata);
    MESHX_ASSERT(MESHX_IS_BEARER_ADV_PKT_TYPE_VALID(pkt_type));

    if (MESHX_BEARER_TYPE_ADV != bearer->type)
    {
        MESHX_ERROR("send failed: invalid bearer type(%d)", bearer->type);
        return -MESHX_ERR_INVAL;
    }

    if (len > MESHX_GAP_ADV_DATA_MAX_LEN - 2)
    {
        MESHX_ERROR("invalid length: %d", len);
        return -MESHX_ERR_LENGTH;
    }
    meshx_bearer_adv_pkt_t adv_data;
    adv_data.length = len + 1;
    adv_data.ad_type = pkt_type;
    memcpy(adv_data.pdu, pdata, len);

    MESHX_INFO("send adv data:  ");
    MESHX_DUMP_INFO(adv_data.buffer, len + 2);

    meshx_gap_action_t action;
    action.action_type = MESHX_GAP_ACTION_TYPE_ADV;
    action.action_adv_data.send_times = repeat_times + 1;
    action.action_adv_data.adv_type = MESHX_GAP_ADV_TYPE_NONCONN_IND;
    memcpy(action.action_adv_data.data, adv_data.buffer, len + 2);
    action.action_adv_data.data_len = len + 2;

    return meshx_gap_add_action(&action);
}

int32_t meshx_bearer_adv_receive(meshx_bearer_t bearer, uint8_t adv_type, const uint8_t *pdata,
                                 uint8_t len, const meshx_bearer_rx_metadata_adv_t *padv_metadata)
{
    MESHX_ASSERT(NULL != bearer);
    int32_t ret = MESHX_SUCCESS;
    switch (adv_type)
    {
    case MESHX_GAP_ADTYPE_MESH_MSG:
        if (NULL == bearer->network_if)
        {
            MESHX_WARN("adv bearer(0x%08x) hasn't connected to any network interface!", bearer);
            ret = -MESHX_ERR_CONNECT;
            break;
        }
        ret = meshx_network_receive(bearer->network_if, pdata, len);
        break;
    case MESHX_GAP_ADTYPE_PB_ADV:
        ret = meshx_provision_receive(bearer, pdata, len);
        break;
    case MESHX_GAP_ADTYPE_MESH_BEACON:
        ret = meshx_beacon_receive(bearer, pdata, len, padv_metadata);
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
    return (MESHX_BEARER_TYPE_INVALID == bearer_adv.bearer.type) ? NULL : &bearer_adv.bearer;
}
