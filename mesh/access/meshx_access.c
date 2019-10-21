/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_ACCESS"
#include "meshx_access.h"
#include "meshx_errno.h"
#include "meshx_trace.h"
#include "meshx_assert.h"
#include "meshx_upper_transport.h"

#define MESHX_UNSEG_ACCESS_MAX_PDU_SIZE                11
#define MESHX_MAX_ACCESS_PDU_SIZE                      380

int32_t meshx_access_init(void)
{
    return MESHX_SUCCESS;
}

int32_t meshx_access_send(meshx_network_if_t network_if,
                          const uint8_t *pdata, uint16_t len, meshx_msg_ctx_t *pmsg_tx_ctx)
{
    /* check address */
    if (!MESHX_ADDRESS_IS_VALID(pmsg_tx_ctx->src) || !MESHX_ADDRESS_IS_VALID(pmsg_tx_ctx->dst))
    {
        MESHX_ERROR("invalid address: src 0x%04x, dst 0x%04x", pmsg_tx_ctx->src, pmsg_tx_ctx->dst);
        return -MESHX_ERR_INVAL;
    }

    /* check parameters length */
    if ((len <= MESHX_UNSEG_ACCESS_MAX_PDU_SIZE) && (0 == pmsg_tx_ctx->seg) &&
        (pmsg_tx_ctx->szmic))
    {
        /* unsegment access message TransMIC fixed to 32bits */
        MESHX_ERROR("unsegment access message TransMIC fixed to 32bits");
        return -MESHX_ERR_INVAL;
    }

    if (pmsg_tx_ctx->szmic && (len > MESHX_MAX_ACCESS_PDU_SIZE - 4))
    {
        MESHX_ERROR("access message exceed maximum size: %d", MESHX_MAX_ACCESS_PDU_SIZE - 4);
        return -MESHX_ERR_LENGTH;
    }
    if ((0 == pmsg_tx_ctx->szmic) && (len > MESHX_MAX_ACCESS_PDU_SIZE))
    {
        MESHX_ERROR("access message exceed maximum size: %d", MESHX_MAX_ACCESS_PDU_SIZE);
        return -MESHX_ERR_LENGTH;
    }

    return meshx_upper_transport_send(network_if, pdata, len, pmsg_tx_ctx);
}

int32_t meshx_access_receive(meshx_network_if_t network_if,
                             const uint8_t *pdata,
                             uint8_t len, meshx_msg_ctx_t *pmsg_rx_ctx)
{
    return MESHX_SUCCESS;
}

