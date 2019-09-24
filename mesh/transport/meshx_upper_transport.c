/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_UPPER_TRANSPORT"
#include "meshx_upper_transport.h"
#include "meshx_lower_transport.h"
#include "meshx_errno.h"
#include "meshx_key.h"
#include "meshx_security.h"

#define MESHX_UNSEG_ACCESS_MAX_PDU_SIZE                    15
#define MESHX_UPPER_TRANPORT_MAX_CTL_PDU_SIZE              256
#define MESHX_UPPER_TRANPORT_MAX_ACCESS_PDU_SIZE           380

int32_t meshx_upper_transport_init(void)
{
    return MESHX_SUCCESS;
}

static void meshx_upper_transport_encrypt(meshx_network_pdu_t *pnet_pdu, uint8_t trans_pdu_len,
                                          uint32_t iv_index, const meshx_application_key_t *papp_key)
{
    meshx_application_nonce_t app_nonce;
    net_nonce.nonce_type = MESHX_NONCE_TYPE_NETWORK;
    net_nonce.ctl = pnet_pdu->net_metadata.ctl;
    net_nonce.ttl = pnet_pdu->net_metadata.ttl;
    net_nonce.seq[0] = pnet_pdu->net_metadata.seq[0];
    net_nonce.seq[1] = pnet_pdu->net_metadata.seq[1];
    net_nonce.seq[2] = pnet_pdu->net_metadata.seq[2];
    net_nonce.src = pnet_pdu->net_metadata.src;
    net_nonce.pad = 0;
    net_nonce.iv_index = MESHX_HOST_TO_BE32(iv_index);
    MESHX_DEBUG("network nonce:");
    MESHX_DUMP_DEBUG(&net_nonce, sizeof(meshx_network_nonce_t));

    uint8_t net_mic_len = pnet_pdu->net_metadata.ctl ? 8 : 4;
    /* encrypt data */
    meshx_aes_ccm_encrypt(pnet_key->encryption_key, (const uint8_t *)&net_nonce,
                          sizeof(meshx_network_nonce_t),
                          NULL, 0, (const uint8_t *)pnet_pdu + MESHX_NETWORK_ENCRYPT_OFFSET,
                          sizeof(pnet_pdu->net_metadata.dst) + trans_pdu_len,
                          (uint8_t *)pnet_pdu + MESHX_NETWORK_ENCRYPT_OFFSET, pnet_pdu->pdu + trans_pdu_len, net_mic_len);
    MESHX_DEBUG("encrypt and auth trans pdu:");
    MESHX_DUMP_DEBUG((uint8_t *)pnet_pdu + MESHX_NETWORK_ENCRYPT_OFFSET,
                     sizeof(pnet_pdu->net_metadata.dst) + trans_pdu_len + net_mic_len);
}

int32_t meshx_upper_transport_send(meshx_network_if_t network_if,
                                   const uint8_t *pdata, uint16_t len,
                                   const meshx_msg_tx_ctx_t *pmsg_tx_ctx)
{
    if (pmsg_tx_ctx->ctl)
    {
        if (len > MESHX_UPPER_TRANPORT_MAX_CTL_PDU_SIZE)
        {
            MESHX_ERROR("control message exceed maximum size: %d", MESHX_UPPER_TRANPORT_MAX_CTL_PDU_SIZE);
            return -MESHX_ERR_LENGTH;
        }
        meshx_lower_transport_send(network_if, pdata, len, pmsg_tx_ctx);
    }
    else
    {
        if ((len <= MESHX_UNSEG_ACCESS_MAX_PDU_SIZE) && (0 == pmsg_tx_ctx->force_seg) &&
            (pmsg_tx_ctx->szmic))
        {
            /* unsegment access message TransMIC fixed to 32bits */
            MESHX_ERROR("unsegment access message TransMIC fixed to 32bits");
            return -MESHX_ERR_INVAL;
        }

        if (pmsg_tx_ctx->szmic && (len > MESHX_UPPER_TRANPORT_MAX_ACCESS_PDU_SIZE - 4))
        {
            MESHX_ERROR("access message exceed maximum size: %d", MESHX_UPPER_TRANPORT_MAX_ACCESS_PDU_SIZE - 4);
            return -MESHX_ERR_LENGTH;
        }

        if ((0 == pmsg_tx_ctx->szmic) && (len > MESHX_UPPER_TRANPORT_MAX_ACCESS_PDU_SIZE))
        {
            MESHX_ERROR("access message exceed maximum size: %d", MESHX_UPPER_TRANPORT_MAX_ACCESS_PDU_SIZE);
            return -MESHX_ERR_LENGTH;
        }

        uint8_t trans_mic_len = 4;
        if (pmsg_tx_ctx->szmic)
        {
            trans_mic_len += 4;
        }

        uint8_t pdu[MESHX_UPPER_TRANPORT_MAX_ACCESS_PDU_SIZE + 4];
        /* encrypt and authenticate access pdu */

    }

    return MESHX_SUCCESS;
}

int32_t meshx_upper_transport_receive(meshx_network_if_t network_if,
                                      const uint8_t *pdata,
                                      uint8_t len, meshx_msg_rx_ctx_t *pmsg_rx_ctx)
{
    return MESHX_SUCCESS;
}

