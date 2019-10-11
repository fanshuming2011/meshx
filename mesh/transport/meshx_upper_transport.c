/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_UPPER_TRANSPORT"
#include "meshx_trace.h"
#include "meshx_upper_transport.h"
#include "meshx_lower_transport.h"
#include "meshx_errno.h"
#include "meshx_key.h"
#include "meshx_security.h"
#include "meshx_endianness.h"

#define MESHX_UNSEG_ACCESS_MAX_PDU_SIZE                    15
#define MESHX_UPPER_TRANPORT_MAX_CTL_PDU_SIZE              256
#define MESHX_UPPER_TRANPORT_MAX_ACCESS_PDU_SIZE           380

int32_t meshx_upper_transport_init(void)
{
    return MESHX_SUCCESS;
}

static void meshx_upper_transport_encrypt(uint8_t *paccess_pdu, uint8_t pdu_len,
                                          uint8_t *ptrans_mic, uint8_t trans_mic_len,
                                          const meshx_msg_tx_ctx_t *pmsg_tx_ctx)
{
    uint8_t nonce[MESHX_NONCE_SIZE];
    if (pmsg_tx_ctx->akf)
    {
        meshx_application_nonce_t *papp_nonce = (meshx_application_nonce_t *)nonce;
        papp_nonce->nonce_type = MESHX_NONCE_TYPE_APPLICAITON;
        papp_nonce->pad = 0;
        papp_nonce->aszmic = pmsg_tx_ctx->szmic;
        papp_nonce->seq[0] = pmsg_tx_ctx->seq >> 16;
        papp_nonce->seq[1] = pmsg_tx_ctx->seq >> 8;
        papp_nonce->seq[2] = pmsg_tx_ctx->seq;
        papp_nonce->src = MESHX_HOST_TO_BE16(pmsg_tx_ctx->src);
        papp_nonce->dst = MESHX_HOST_TO_BE16(pmsg_tx_ctx->dst);
        papp_nonce->iv_index = MESHX_HOST_TO_BE32(pmsg_tx_ctx->iv_index);
        MESHX_DEBUG("application nonce:");
        MESHX_DUMP_DEBUG(papp_nonce, sizeof(meshx_application_nonce_t));
    }
    else
    {
        meshx_device_nonce_t *pdev_nonce = (meshx_device_nonce_t *)nonce;
        pdev_nonce->nonce_type = MESHX_NONCE_TYPE_DEVICE;
        pdev_nonce->pad = 0;
        pdev_nonce->aszmic = pmsg_tx_ctx->szmic;
        pdev_nonce->seq[0] = pmsg_tx_ctx->seq >> 16;
        pdev_nonce->seq[1] = pmsg_tx_ctx->seq >> 8;
        pdev_nonce->seq[2] = pmsg_tx_ctx->seq;
        pdev_nonce->src = MESHX_HOST_TO_BE16(pmsg_tx_ctx->src);
        pdev_nonce->dst = MESHX_HOST_TO_BE16(pmsg_tx_ctx->dst);
        pdev_nonce->iv_index = MESHX_HOST_TO_BE32(pmsg_tx_ctx->iv_index);
        MESHX_DEBUG("device nonce:");
        MESHX_DUMP_DEBUG(&pdev_nonce, sizeof(meshx_device_nonce_t));
    }

    /* TODO: label uuid */
    uint8_t *padd = NULL;
    uint8_t add_len = 0;

    const meshx_key_t *pencrypt_key = pmsg_tx_ctx->akf ? pmsg_tx_ctx->papp_key :
                                      pmsg_tx_ctx->pdev_key;
    /* encrypt data */
    meshx_aes_ccm_encrypt(*pencrypt_key, nonce, MESHX_NONCE_SIZE,
                          padd, add_len, paccess_pdu, pdu_len, paccess_pdu, ptrans_mic, trans_mic_len);
    MESHX_DEBUG("encrypt access pdu:");
    MESHX_DUMP_DEBUG(paccess_pdu, pdu_len);
    MESHX_DEBUG("access pdu transmic:");
    MESHX_DUMP_DEBUG(ptrans_mic, trans_mic_len);
}

int32_t meshx_upper_transport_send(meshx_network_if_t network_if,
                                   const uint8_t *pdata, uint16_t len,
                                   const meshx_msg_tx_ctx_t *pmsg_tx_ctx)
{
    int32_t ret = MESHX_SUCCESS;
    if (pmsg_tx_ctx->ctl)
    {
        if (len > MESHX_UPPER_TRANPORT_MAX_CTL_PDU_SIZE)
        {
            MESHX_ERROR("control message exceed maximum size: %d", MESHX_UPPER_TRANPORT_MAX_CTL_PDU_SIZE);
            return -MESHX_ERR_LENGTH;
        }
        ret = meshx_lower_transport_send(network_if, pdata, len, pmsg_tx_ctx);
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
        meshx_upper_transport_encrypt(pdu, len, pdu + len, trans_mic_len, pmsg_tx_ctx);

        ret = meshx_lower_transport_send(network_if, pdu, len + trans_mic_len, pmsg_tx_ctx);
    }

    return ret;
}

int32_t meshx_upper_transport_receive(meshx_network_if_t network_if,
                                      const uint8_t *pdata,
                                      uint8_t len, meshx_msg_rx_ctx_t *pmsg_rx_ctx)
{
    return MESHX_SUCCESS;
}

