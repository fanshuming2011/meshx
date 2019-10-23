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
#include "meshx_node_internal.h"

#define MESHX_UNSEG_ACCESS_MAX_PDU_SIZE                11
#define MESHX_MAX_ACCESS_PDU_SIZE                      380

#define MESHX_ACCESS_OPCODE_RFU                        0x7F

#define MESHX_ACCESS_1BYTE_FLAG                        0x80

int32_t meshx_access_init(void)
{
    return MESHX_SUCCESS;
}

void meshx_access_opcode_to_buf(uint32_t opcode, uint8_t *pdata)
{
    if (NULL != pdata)
    {
        if (1 == MESHX_ACCESS_OPCODE_SIZE(opcode))
        {
            pdata[0] = opcode;
        }
        else if (2 == MESHX_ACCESS_OPCODE_SIZE(opcode))
        {
            pdata[0] = ((opcode >> 8) & 0xff);
            pdata[1] = (opcode & 0xff);
        }
        else
        {
            pdata[0] = ((opcode >> 16) & 0xff);
            pdata[1] = ((opcode >> 8) & 0xff);
            pdata[2] = (opcode & 0xff);
        }
    }
}

uint32_t meshx_access_buf_to_opcode(const uint8_t *pdata)
{
    if (NULL == pdata)
    {
        return MESHX_ACCESS_OPCODE_RFU;
    }

    uint32_t opcode = 0;
    if (0 == (pdata[0] & 0x80))
    {
        opcode = pdata[0];
    }
    else if (0 == (pdata[0] & 0x40))
    {
        opcode = pdata[0];
        opcode <<= 8;
        opcode |= pdata[1];
    }
    else
    {
        opcode = pdata[0];
        opcode <<= 8;
        opcode |= pdata[1];
        opcode <<= 8;
        opcode |= pdata[2];
    }

    return opcode;
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

    /* check keys, because app key and dev key is union, so only check one */
    if ((NULL == pmsg_tx_ctx->pnet_key) || (NULL == pmsg_tx_ctx->papp_key))
    {
        MESHX_ERROR("invalid key: net key 0x%08x, app key or dev key 0x%08x", pmsg_tx_ctx->pnet_key,
                    pmsg_tx_ctx->papp_key);
        return -MESHX_ERR_KEY;
    }

    if ((pmsg_tx_ctx->akf && (0 == pmsg_tx_ctx->aid)) || ((!pmsg_tx_ctx->akf) &&
                                                          (0 != pmsg_tx_ctx->aid)))
    {
        MESHX_ERROR("invalid akf(%d) and aid(0x%x)", pmsg_tx_ctx->akf, pmsg_tx_ctx->aid);
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
    MESHX_INFO("access message: src 0x%04x, dst 0x%04x, ttl %d, seq-seq auth 0x%06x-0x%06x, iv index 0x%08x, seg %d, akf %d, nid %d, aid %d",
               pmsg_rx_ctx->src, pmsg_rx_ctx->dst, pmsg_rx_ctx->ttl, pmsg_rx_ctx->seq, pmsg_rx_ctx->seq_auth,
               pmsg_rx_ctx->iv_index, pmsg_rx_ctx->seg, pmsg_rx_ctx->akf, pmsg_rx_ctx->pnet_key->nid,
               pmsg_rx_ctx->aid);
    MESHX_DUMP_INFO(pdata, len);
    return MESHX_SUCCESS;
}

