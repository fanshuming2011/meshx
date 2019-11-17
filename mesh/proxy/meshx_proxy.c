/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_PROXY"
#include "meshx_proxy.h"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_bearer_internal.h"
#include "meshx_mem.h"
#include "meshx_list.h"
#include "meshx_timer.h"
#include "meshx_async.h"
#include "meshx_async_internal.h"
#include "meshx_net.h"
#include "meshx_beacon.h"

#define MESHX_PROXY_INIT_PDU_LEN                   80

#define MESHX_PROXY_SAR_TRANSFER_TIME              20000

typedef struct
{
    uint8_t msg_type : 6;
    uint8_t sar : 2;
    uint8_t data[0];
} __PACKED meshx_proxy_pdu_t;

#define MESHX_PROXY_SAR_COMPLETE_MSG                0
#define MESHX_PROXY_SAR_FIRST_SEG                   1
#define MESHX_PROXY_SAR_CONTINUE_SEG                2
#define MESHX_PROXY_SAR_LAST_SEG                    3

typedef struct
{
    meshx_bearer_t bearer;
    uint8_t msg_type;
    uint8_t *pdata;
    uint16_t data_len;
    uint16_t max_len;
    meshx_timer_t sar_timer;
    meshx_list_t node;
} meshx_proxy_rx_ctx_t;

static meshx_list_t meshx_proxy_rx_ctx_list;

int32_t meshx_proxy_init(void)
{
    meshx_list_init_head(&meshx_proxy_rx_ctx_list);
    return MESHX_SUCCESS;
}

static meshx_proxy_rx_ctx_t *meshx_proxy_find_rx_ctx(meshx_bearer_t bearer)
{
    meshx_list_t *pnode;
    meshx_proxy_rx_ctx_t *pctx;
    meshx_list_foreach(pnode, &meshx_proxy_rx_ctx_list)
    {
        pctx = MESHX_CONTAINER_OF(pnode, meshx_proxy_rx_ctx_t, node);
        if (pctx->bearer == bearer)
        {
            return pctx;
        }
    }

    return NULL;
}

static void meshx_proxy_rx_ctx_release(meshx_proxy_rx_ctx_t *prx_ctx)
{
    MESHX_ASSERT(NULL != prx_ctx);
    if (NULL != prx_ctx->pdata)
    {
        meshx_free(prx_ctx->pdata);
    }

    if (NULL != prx_ctx->sar_timer)
    {
        meshx_timer_delete(prx_ctx->sar_timer);
    }

    meshx_list_remove(&prx_ctx->node);
    meshx_free(prx_ctx);
}

static void meshx_proxy_sar_timeout_handler(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_PROXY_SAR;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

void meshx_proxy_async_handle_sar_timeout(meshx_async_msg_t msg)
{
    meshx_proxy_rx_ctx_t *prx_ctx = msg.pdata;
    meshx_proxy_rx_ctx_release(prx_ctx);
    MESHX_WARN("receive msg type(%d) data timeout!", prx_ctx->msg_type);
}

static meshx_proxy_rx_ctx_t *meshx_proxy_rx_ctx_request(uint16_t len)
{
    meshx_proxy_rx_ctx_t *prx_ctx = meshx_malloc(sizeof(meshx_proxy_rx_ctx_t));
    if (NULL == prx_ctx)
    {
        MESHX_ERROR("allocate memory for rx ctx failed!");
        return NULL;
    }
    memset(prx_ctx, 0, sizeof(meshx_proxy_rx_ctx_t));

    prx_ctx->max_len = MESHX_PROXY_INIT_PDU_LEN;
    if (len > MESHX_PROXY_INIT_PDU_LEN)
    {
        prx_ctx->pdata = meshx_malloc(len + MESHX_PROXY_INIT_PDU_LEN);
        prx_ctx->max_len += len;
    }
    else
    {
        prx_ctx->pdata = meshx_malloc(MESHX_PROXY_INIT_PDU_LEN);
    }

    if (NULL == prx_ctx->pdata)
    {
        MESHX_ERROR("allocate memory for rx ctx data failed!");
        meshx_free(prx_ctx);
        return NULL;
    }

    meshx_timer_create(&prx_ctx->sar_timer, MESHX_TIMER_MODE_SINGLE_SHOT,
                       meshx_proxy_sar_timeout_handler, prx_ctx);
    if (NULL == prx_ctx->sar_timer)
    {
        MESHX_ERROR("create sar timer failed!");
        meshx_free(prx_ctx->pdata);
        meshx_free(prx_ctx);
        return NULL;
    }

    meshx_list_append(&meshx_proxy_rx_ctx_list, &prx_ctx->node);

    return prx_ctx;
}

static int32_t meshx_proxy_rx_ctx_expand(meshx_proxy_rx_ctx_t *prx_ctx, uint16_t len)
{
    MESHX_ASSERT(NULL != prx_ctx);
    uint8_t *pdata = meshx_malloc(prx_ctx->data_len + len + MESHX_PROXY_INIT_PDU_LEN);
    if (NULL == pdata)
    {
        MESHX_ERROR("expand pdu len failed, out of memeory!");
        return -MESHX_ERR_MEM;
    }

    if (NULL != prx_ctx->pdata)
    {
        memcpy(pdata, prx_ctx->pdata, prx_ctx->data_len);
        meshx_free(prx_ctx->pdata);
    }
    prx_ctx->max_len += (len + MESHX_PROXY_INIT_PDU_LEN);

    MESHX_INFO("expand ctx pdu len to %d", prx_ctx->max_len);
    prx_ctx->pdata = pdata;

    return MESHX_SUCCESS;
}

int32_t meshx_proxy_send(meshx_bearer_t bearer, uint8_t msg_type, const uint8_t *pdata,
                         uint16_t len)
{
    uint16_t mtu = meshx_bearer_gatt_mtu_get(bearer);
    if (mtu > len)
    {
        /* no segment */
        uint16_t msg_len = sizeof(meshx_proxy_pdu_t) + len;
        meshx_proxy_pdu_t *ppdu = meshx_malloc(msg_len);
        if (NULL == ppdu)
        {
            MESHX_ERROR("proxy send failed, out of memory: %d", msg_len);
            return -MESHX_ERR_MEM;
        }
        ppdu->sar = MESHX_PROXY_SAR_COMPLETE_MSG;
        ppdu->msg_type = msg_type;
        memcpy(ppdu->data, pdata, len);

        meshx_bearer_gatt_send(bearer, (const uint8_t *)ppdu, msg_len);

        meshx_free(ppdu);
    }
    else
    {
        /* segment */
        meshx_proxy_pdu_t *ppdu = meshx_malloc(sizeof(meshx_proxy_pdu_t) + mtu);
        if (NULL == ppdu)
        {
            MESHX_ERROR("proxy send failed, out of memory: %d", sizeof(meshx_proxy_pdu_t) + mtu);
            return -MESHX_ERR_MEM;
        }

        uint16_t seg_num = len / mtu + 1;
        uint16_t seg_len = 0;
        uint16_t seg_offset = 0;
        ppdu->msg_type = msg_type;
        for (uint16_t i = 0; i < seg_num; ++i)
        {
            ppdu->sar = (i == 0) ? MESHX_PROXY_SAR_FIRST_SEG : ((i == (seg_num - 1)) ?
                                                                MESHX_PROXY_SAR_LAST_SEG : MESHX_PROXY_SAR_CONTINUE_SEG);
            seg_len = (i == (seg_num - 1)) ? (len - seg_offset) : mtu ;
            memcpy(ppdu->data, pdata + seg_offset, seg_len);
            meshx_bearer_gatt_send(bearer, (const uint8_t *)ppdu, sizeof(meshx_proxy_pdu_t) + seg_len);
            seg_offset += seg_len;
        }

        meshx_free(ppdu);
    }

    return MESHX_SUCCESS;
}

static int32_t meshx_proxy_dispatch(meshx_bearer_t bearer, uint8_t msg_type, const uint8_t *pdata,
                                    uint16_t len)
{
    int32_t ret = MESHX_SUCCESS;

    switch (msg_type)
    {
    case MESHX_PROXY_MSG_TYPE_NET:
        {
            meshx_net_iface_t net_iface = meshx_net_iface_get(bearer);
            if (NULL == net_iface)
            {
                MESHX_ERROR("bearer(0x%08x) has not been connected to any network interface!", bearer);
                ret = -MESHX_ERR_CONNECT;
            }
            else
            {
                ret = meshx_net_receive(net_iface, pdata, len);
            }
        }
        break;
    case MESHX_PROXY_MSG_TYPE_BEACON:
        ret = meshx_beacon_receive(bearer, pdata, len, NULL);
        break;
    case MESHX_PROXY_MSG_TYPE_PROXY_CFG:
        break;
    case MESHX_PROXY_MSG_TYPE_PROV:
        break;
    default:
        ret = -MESHX_ERR_INVAL;
        MESHX_ERROR("unkonwn msg type: %d", msg_type);
        break;
    }

    return ret;
}

int32_t meshx_proxy_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint16_t len)
{
    if (len <= sizeof(meshx_proxy_pdu_t))
    {
        MESHX_ERROR("length too short: %d", len);
        return -MESHX_ERR_LENGTH;
    }

    const meshx_proxy_pdu_t *prx_pdu = (meshx_proxy_pdu_t *)pdata;
    meshx_proxy_rx_ctx_t *prx_ctx = meshx_proxy_find_rx_ctx(bearer);
    uint16_t data_len = len - sizeof(meshx_proxy_pdu_t);
    if (NULL == prx_ctx)
    {
        if ((MESHX_PROXY_SAR_COMPLETE_MSG != prx_pdu->sar) &&
            (MESHX_PROXY_SAR_FIRST_SEG != prx_pdu->sar))
        {
            /* unexpected value of SAR field */
            MESHX_ERROR("receive unexpected sar field: %d", prx_pdu->msg_type);
            /* TODO: disconnect link? */
            return -MESHX_ERR_UNEXPECTED;
        }

        prx_ctx = meshx_proxy_rx_ctx_request(len);
        if (NULL == prx_ctx)
        {
            MESHX_ERROR("can't process proxy now, out of resource");
            return -MESHX_ERR_MEM;
        }
        prx_ctx->bearer = bearer;
        prx_ctx->msg_type = prx_pdu->msg_type;
        memcpy(prx_ctx->pdata, prx_pdu->data, data_len);
        prx_ctx->data_len = data_len;
        /* start timeout timer */
        meshx_timer_start(prx_ctx->sar_timer, MESHX_PROXY_SAR_TRANSFER_TIME);
    }
    else
    {
        if ((MESHX_PROXY_SAR_COMPLETE_MSG == prx_pdu->sar) ||
            (MESHX_PROXY_SAR_FIRST_SEG == prx_pdu->sar))
        {
            /* unexpected value of SAR field */
            MESHX_ERROR("receive unexpected sar field: %d", prx_pdu->msg_type);
            meshx_proxy_rx_ctx_release(prx_ctx);
            /* TODO: disconnect link? */
            return -MESHX_ERR_UNEXPECTED;
        }

        if (prx_ctx->msg_type != prx_pdu->msg_type)
        {
            MESHX_ERROR("receive unexpected type field: %d-%d", prx_ctx->msg_type, prx_pdu->msg_type);
            meshx_proxy_rx_ctx_release(prx_ctx);
            /* TODO: disconnect link? */
            return -MESHX_ERR_UNEXPECTED;
        }

        /* copy data */
        if (prx_ctx->data_len + data_len > prx_ctx->max_len)
        {
            if (MESHX_SUCCESS != meshx_proxy_rx_ctx_expand(prx_ctx, data_len))
            {
                MESHX_ERROR("can't receive fragment, out of memory!");
                meshx_proxy_rx_ctx_release(prx_ctx);
                /* TODO: disconnect link? */
                return -MESHX_ERR_MEM;
            }
        }
        memcpy(prx_ctx->pdata + prx_ctx->data_len, prx_pdu->data, data_len);
        prx_ctx->data_len += data_len;

        if (MESHX_PROXY_SAR_LAST_SEG == prx_pdu->sar)
        {
            /* received all segments, notify upper layer */
            meshx_proxy_dispatch(bearer, prx_ctx->msg_type, prx_ctx->pdata, prx_ctx->data_len);
            /* free ctx */
            meshx_proxy_rx_ctx_release(prx_ctx);
        }
    }

    return MESHX_SUCCESS;
}

