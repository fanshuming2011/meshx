/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "MESHX_UPPER_TRANS"
#include <string.h>
#include "meshx_trace.h"
#include "meshx_upper_trans.h"
#include "meshx_lower_trans.h"
#include "meshx_errno.h"
#include "meshx_key.h"
#include "meshx_security.h"
#include "meshx_endianness.h"
#include "meshx_node_internal.h"
#include "meshx_access.h"
#include "meshx_mem.h"
#include "meshx_seq.h"

#define MESHX_UNSEG_ACCESS_MAX_PDU_SIZE                    15
#define MESHX_MAX_CTL_PDU_SIZE                             256

int32_t meshx_upper_trans_init(void)
{
    return MESHX_SUCCESS;
}

static void meshx_upper_trans_encrypt(uint8_t *paccess_pdu, uint8_t pdu_len,
                                      uint8_t *ptrans_mic, uint8_t trans_mic_len,
                                      const meshx_msg_ctx_t *pmsg_tx_ctx)
{
    uint8_t nonce[MESHX_NONCE_SIZE];
    if (pmsg_tx_ctx->akf)
    {
        meshx_app_nonce_t *papp_nonce = (meshx_app_nonce_t *)nonce;
        papp_nonce->nonce_type = MESHX_NONCE_TYPE_APP;
        papp_nonce->pad = 0;
        papp_nonce->aszmic = pmsg_tx_ctx->szmic;
        papp_nonce->seq[0] = pmsg_tx_ctx->seq_auth >> 16;
        papp_nonce->seq[1] = pmsg_tx_ctx->seq_auth >> 8;
        papp_nonce->seq[2] = pmsg_tx_ctx->seq_auth;
        papp_nonce->src = MESHX_HOST_TO_BE16(pmsg_tx_ctx->src);
        papp_nonce->dst = MESHX_HOST_TO_BE16(pmsg_tx_ctx->dst);
        papp_nonce->iv_index = MESHX_HOST_TO_BE32(pmsg_tx_ctx->iv_index);
        MESHX_DEBUG("application nonce:");
        MESHX_DUMP_DEBUG(papp_nonce, sizeof(meshx_app_nonce_t));
    }
    else
    {
        meshx_device_nonce_t *pdev_nonce = (meshx_device_nonce_t *)nonce;
        pdev_nonce->nonce_type = MESHX_NONCE_TYPE_DEVICE;
        pdev_nonce->pad = 0;
        pdev_nonce->aszmic = pmsg_tx_ctx->szmic;
        pdev_nonce->seq[0] = pmsg_tx_ctx->seq_auth >> 16;
        pdev_nonce->seq[1] = pmsg_tx_ctx->seq_auth >> 8;
        pdev_nonce->seq[2] = pmsg_tx_ctx->seq_auth;
        pdev_nonce->src = MESHX_HOST_TO_BE16(pmsg_tx_ctx->src);
        pdev_nonce->dst = MESHX_HOST_TO_BE16(pmsg_tx_ctx->dst);
        pdev_nonce->iv_index = MESHX_HOST_TO_BE32(pmsg_tx_ctx->iv_index);
        MESHX_DEBUG("device nonce:");
        MESHX_DUMP_DEBUG(pdev_nonce, sizeof(meshx_device_nonce_t));
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

static int32_t meshx_upper_trans_decrypt(uint8_t *paccess_pdu, uint8_t pdu_len,
                                         uint8_t *ptrans_mic, uint8_t trans_mic_len,
                                         meshx_msg_ctx_t *pmsg_rx_ctx)
{
    int32_t ret = MESHX_SUCCESS;
    uint8_t nonce[MESHX_NONCE_SIZE];
    if (pmsg_rx_ctx->akf)
    {
        meshx_app_nonce_t *papp_nonce = (meshx_app_nonce_t *)nonce;
        papp_nonce->nonce_type = MESHX_NONCE_TYPE_APP;
        papp_nonce->pad = 0;
        /* TODO: only valid in segment access message, other message shall be 0 */
        papp_nonce->aszmic = pmsg_rx_ctx->szmic;
        papp_nonce->seq[0] = pmsg_rx_ctx->seq_auth >> 16;
        papp_nonce->seq[1] = pmsg_rx_ctx->seq_auth >> 8;
        papp_nonce->seq[2] = pmsg_rx_ctx->seq_auth;
        papp_nonce->src = MESHX_HOST_TO_BE16(pmsg_rx_ctx->src);
        papp_nonce->dst = MESHX_HOST_TO_BE16(pmsg_rx_ctx->dst);
        papp_nonce->iv_index = MESHX_HOST_TO_BE32(pmsg_rx_ctx->iv_index);
        MESHX_DEBUG("application nonce:");
        MESHX_DUMP_DEBUG(papp_nonce, sizeof(meshx_app_nonce_t));

        /* get app key */
        int32_t ret = MESHX_SUCCESS;
        /* TODO: label uuid */
        uint8_t *padd = NULL;
        uint8_t add_len = 0;
        meshx_app_key_t *papp_key = NULL;
        meshx_app_key_traverse_start(&papp_key);
        while (NULL != papp_key)
        {
            uint8_t loop = 1;
            if ((MESHX_KEY_STATE_PHASE1 == papp_key->key_state) ||
                (MESHX_KEY_STATE_PHASE2 == papp_key->key_state))
            {
                loop = 2;
            }

            for (uint8_t i = 0; i < loop; ++i)
            {
                if (papp_key->key_value[loop].aid == pmsg_rx_ctx->aid)
                {
                    ret = meshx_aes_ccm_decrypt(papp_key->key_value[loop].app_key, nonce, MESHX_NONCE_SIZE,
                                                padd, add_len, paccess_pdu, pdu_len, paccess_pdu, ptrans_mic, trans_mic_len);
                    if (MESHX_SUCCESS == ret)
                    {
                        pmsg_rx_ctx->papp_key = &papp_key->key_value[loop].app_key;
                        goto FINISH;
                    }
                }
            }

            meshx_app_key_traverse_continue(&papp_key);
        }

FINISH:
        if (NULL == papp_key)
        {
            MESHX_WARN("can't decrypt pdu by application key that aid is 0x%x", pmsg_rx_ctx->aid);
            return -MESHX_ERR_KEY;
        }
        MESHX_DEBUG("decrypt access pdu:");
        MESHX_DUMP_DEBUG(paccess_pdu, pdu_len);
    }
    else
    {
        meshx_device_nonce_t *pdev_nonce = (meshx_device_nonce_t *)nonce;
        pdev_nonce->nonce_type = MESHX_NONCE_TYPE_DEVICE;
        pdev_nonce->pad = 0;
        /* TODO: only valid in segment access message, other message shall be 0 */
        pdev_nonce->aszmic = pmsg_rx_ctx->szmic;
        pdev_nonce->seq[0] = pmsg_rx_ctx->seq_auth >> 16;
        pdev_nonce->seq[1] = pmsg_rx_ctx->seq_auth >> 8;
        pdev_nonce->seq[2] = pmsg_rx_ctx->seq_auth;
        pdev_nonce->src = MESHX_HOST_TO_BE16(pmsg_rx_ctx->src);
        pdev_nonce->dst = MESHX_HOST_TO_BE16(pmsg_rx_ctx->dst);
        pdev_nonce->iv_index = MESHX_HOST_TO_BE32(pmsg_rx_ctx->iv_index);
        MESHX_DEBUG("device nonce:");
        MESHX_DUMP_DEBUG(pdev_nonce, sizeof(meshx_device_nonce_t));

        /* TODO: label uuid */
        uint8_t *padd = NULL;
        uint8_t add_len = 0;

        /* decrypt data */
        const meshx_device_key_t *pdev_key = meshx_dev_key_get(meshx_node_params.param.node_addr);
        if (NULL == pdev_key)
        {
            MESHX_ERROR("device key is invalid!");
            return -MESHX_ERR_KEY;
        }
        pmsg_rx_ctx->pdev_key = &pdev_key->dev_key;
        ret = meshx_aes_ccm_decrypt(*pmsg_rx_ctx->pdev_key, nonce, MESHX_NONCE_SIZE,
                                    padd, add_len, paccess_pdu, pdu_len, paccess_pdu, ptrans_mic, trans_mic_len);
        if (MESHX_SUCCESS == ret)
        {
            MESHX_DEBUG("decrypt access pdu:");
            MESHX_DUMP_DEBUG(paccess_pdu, pdu_len);
        }
    }

    return ret;
}

int32_t meshx_upper_trans_send(const uint8_t *pdata, uint16_t len,
                               meshx_msg_ctx_t *pmsg_tx_ctx)
{
    if (NULL != pmsg_tx_ctx->net_iface)
    {
        if (!meshx_net_iface_is_connect(pmsg_tx_ctx->net_iface))
        {
            MESHX_ERROR("network interface is disconnected!");
            return -MESHX_ERR_CONNECT;
        }
    }

    int32_t ret = MESHX_SUCCESS;
    if (pmsg_tx_ctx->ctl)
    {
        if (len > MESHX_MAX_CTL_PDU_SIZE)
        {
            MESHX_ERROR("control message exceed maximum size: %d", MESHX_MAX_CTL_PDU_SIZE);
            return -MESHX_ERR_LENGTH;
        }

        MESHX_INFO("control message: src 0x%04x, dst 0x%04x, ttl %d, seq 0x%06x, iv index 0x%08x, seg %d, opcode 0x%x",
                   pmsg_tx_ctx->src, pmsg_tx_ctx->dst, pmsg_tx_ctx->ttl, pmsg_tx_ctx->seq,
                   pmsg_tx_ctx->iv_index, pmsg_tx_ctx->seg, pmsg_tx_ctx->opcode);
        MESHX_DUMP_INFO(pdata, len);

        ret = meshx_lower_trans_send(pdata, len, pmsg_tx_ctx);
    }
    else
    {
        uint8_t trans_mic_len = 4;
        if (pmsg_tx_ctx->szmic)
        {
            trans_mic_len = 8;
        }

        uint8_t *ppdu = meshx_malloc(len + trans_mic_len);
        if (NULL == ppdu)
        {
            MESHX_ERROR("allocate access pdu data failed!");
            return -MESHX_ERR_MEM;
        }
        memcpy(ppdu, pdata, len);

        /* allocate sequence */
        pmsg_tx_ctx->seq = meshx_seq_use(pmsg_tx_ctx->src - meshx_node_params.param.node_addr);
        pmsg_tx_ctx->seq_auth = pmsg_tx_ctx->seq;

        MESHX_INFO("access message: src 0x%04x, dst 0x%04x, ttl %d, seq auth 0x%06x, iv index 0x%08x, seg %d, akf %d, nid %d, aid %d",
                   pmsg_tx_ctx->src, pmsg_tx_ctx->dst, pmsg_tx_ctx->ttl, pmsg_tx_ctx->seq,
                   pmsg_tx_ctx->iv_index, pmsg_tx_ctx->seg, pmsg_tx_ctx->akf, pmsg_tx_ctx->pnet_key->nid,
                   pmsg_tx_ctx->aid);
        MESHX_DUMP_INFO(pdata, len);

        /* encrypt and authenticate access pdu */
        meshx_upper_trans_encrypt(ppdu, len, ppdu + len, trans_mic_len, pmsg_tx_ctx);

        ret = meshx_lower_trans_send(ppdu, len + trans_mic_len, pmsg_tx_ctx);
        meshx_free(ppdu);
    }

    return ret;
}

int32_t meshx_upper_trans_receive(uint8_t *pdata,
                                  uint8_t len, meshx_msg_ctx_t *pmsg_rx_ctx)
{
    MESHX_DEBUG("receive upper transport pdu: type %d", pmsg_rx_ctx->ctl);
    MESHX_DUMP_DEBUG(pdata, len);

    int32_t ret = MESHX_SUCCESS;
    if (pmsg_rx_ctx->ctl)
    {
        /* receive control message */
    }
    else
    {
        /* receive access message */
        /* decrypt access message */
        uint8_t trans_mic_len = 4;
        if (pmsg_rx_ctx->seg && pmsg_rx_ctx->szmic)
        {
            trans_mic_len = 8;
        }

        ret = meshx_upper_trans_decrypt(pdata, len - trans_mic_len, pdata + len - trans_mic_len,
                                        trans_mic_len, pmsg_rx_ctx);
        if (MESHX_SUCCESS == ret)
        {
            /* notify access layer */
            ret = meshx_access_receive(pdata, len - trans_mic_len, pmsg_rx_ctx);
        }
        else
        {
            MESHX_WARN("decrypt access pdu failed!");
        }
    }

    return ret;
}

