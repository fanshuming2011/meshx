/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_NETWORK"
#include "meshx_trace.h"
#include "meshx_network.h"
#include "meshx_errno.h"
#include "meshx_config.h"
#include "meshx_bearer_internal.h"
#include "meshx_network_internal.h"
#include "meshx_seq.h"
#include "meshx_iv_index.h"
#include "meshx_node.h"
#include "meshx_security.h"
#include "meshx_endianness.h"
#include "meshx_nmc.h"
#include "meshx_rpl.h"

#define MESHX_NETWORK_TRANS_PDU_MAX_LEN         20
#define MESHX_NETWORK_ENCRYPT_OFFSET            7
#define MESHX_NETWORK_OBFUSCATION_OFFSET        1
#define MESHX_NETWORK_OBFUSCATION_SIZE          6

typedef struct
{
    uint8_t nid : 7;
    uint8_t ivi : 1;
    uint8_t ttl : 7;
    uint8_t ctl : 1;
    uint8_t seq[3];
    uint16_t src;
    uint16_t dst;
} __PACKED meshx_network_metadata_t;

typedef struct
{
    meshx_network_metadata_t net_metadata;
    uint8_t pdu[MESHX_NETWORK_TRANS_PDU_MAX_LEN]; /* netmic is 32bit if ctl is 0, nwtmic is 64bit if ctl is 1*/
} __PACKED meshx_network_pdu_t;


int32_t meshx_network_init(void)
{
    meshx_network_if_init();
    return MESHX_SUCCESS;
}

static void meshx_network_encrypt(meshx_network_pdu_t *pnet_pdu, uint8_t trans_pdu_len,
                                  uint32_t iv_index, const meshx_network_key_t *pnet_key)
{
    meshx_network_nonce_t net_nonce;
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

static void meshx_network_obfuscation(meshx_network_pdu_t *pnet_pdu, uint32_t iv_index,
                                      const meshx_network_key_t *pnet_key)
{
    iv_index = MESHX_HOST_TO_BE32(iv_index);
    uint8_t privacy_plaintext[16];
    memset(privacy_plaintext, 0, 5);
    memcpy(privacy_plaintext + 5, &iv_index, 4);
    memcpy(privacy_plaintext + 9, (const uint8_t *)pnet_pdu + MESHX_NETWORK_ENCRYPT_OFFSET, 7);
    MESHX_DEBUG("privacy plaintext:");
    MESHX_DUMP_DEBUG(privacy_plaintext, 16);
    meshx_e(privacy_plaintext, pnet_key->privacy_key, privacy_plaintext);
    MESHX_DEBUG("PECB:");
    MESHX_DUMP_DEBUG(privacy_plaintext, 16);
    uint8_t *pdata = (uint8_t *)pnet_pdu + MESHX_NETWORK_OBFUSCATION_OFFSET;
    for (uint8_t i = 0; i < MESHX_NETWORK_OBFUSCATION_SIZE; ++i)
    {
        *pdata = *pdata ^ privacy_plaintext[i];
        pdata ++;
    }
    MESHX_DEBUG("obfuscation network header:");
    MESHX_DUMP_DEBUG((uint8_t *)pnet_pdu + MESHX_NETWORK_OBFUSCATION_OFFSET,
                     MESHX_NETWORK_OBFUSCATION_SIZE);
}

static int32_t meshx_network_decrypt(meshx_network_pdu_t *pnet_pdu, uint8_t trans_pdu_len,
                                     uint32_t iv_index, const meshx_network_key_t *pnet_key)
{
    meshx_network_nonce_t net_nonce;
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
    /* decrypt data */
    int32_t ret = meshx_aes_ccm_decrypt(pnet_key->encryption_key, (const uint8_t *)&net_nonce,
                                        sizeof(meshx_network_nonce_t),
                                        NULL, 0, (const uint8_t *)pnet_pdu + MESHX_NETWORK_ENCRYPT_OFFSET,
                                        sizeof(pnet_pdu->net_metadata.dst) + trans_pdu_len,
                                        (uint8_t *)pnet_pdu + MESHX_NETWORK_ENCRYPT_OFFSET, pnet_pdu->pdu + trans_pdu_len, net_mic_len);
    if (MESHX_SUCCESS == ret)
    {
        MESHX_DEBUG("dencrypt trans pdu:");
        MESHX_DUMP_DEBUG((uint8_t *)pnet_pdu + MESHX_NETWORK_ENCRYPT_OFFSET,
                         sizeof(pnet_pdu->net_metadata.dst) + trans_pdu_len);
    }
    else
    {
        MESHX_ERROR("decrypt network pdu failed!");
    }

    return ret;
}

int32_t meshx_network_receive(meshx_network_if_t network_if, const uint8_t *pdata, uint8_t len)
{
    if (NULL == network_if)
    {
        MESHX_ERROR("network interface is NULL");
        return -MESHX_ERR_INVAL;
    }

    /* filter data */
    /* TODO: add input filter data, use adv information? */
    meshx_network_if_input_filter_data_t filter_data = {};
    if (!meshx_network_if_input_filter(network_if, &filter_data))
    {
        MESHX_INFO("network data has been filtered!");
        return -MESHX_ERR_FILTER;
    }

    /* copy data */
    uint32_t iv_index = meshx_iv_index_get();
    meshx_network_pdu_t net_pdu = *(const meshx_network_pdu_t *)pdata;

    /* TODO: check ivi, nid and get net key */
    const meshx_network_key_t *pnet_key = meshx_net_key_get_by_nid(net_pdu.net_metadata.nid);
    if (NULL == pnet_key)
    {
        MESHX_INFO("no key's nid is %d", net_pdu.net_metadata.nid);
        return -MESHX_ERR_KEY;
    }

    /* restore header */
    meshx_network_obfuscation(&net_pdu, iv_index, pnet_key);

    /* TODO: check nmc, rpl and relay */
    uint16_t src = MESHX_BE16_TO_HOST(net_pdu.net_metadata.src);
    uint16_t dst = MESHX_BE16_TO_HOST(net_pdu.net_metadata.dst);
    if (!MESHX_ADDRESS_IS_VALID(src) || !MESHX_ADDRESS_IS_VALID(dst))
    {
        MESHX_ERROR("invalid address: src 0x%04x, dst 0x%04x", src, dst);
        return -MESHX_ERR_INVAL;
    }
    uint32_t seq = net_pdu.net_metadata.seq[0];
    seq <<= 8;
    seq |= net_pdu.net_metadata.seq[1];
    seq <<= 8;
    seq |= net_pdu.net_metadata.seq[2];
    if (meshx_node_is_my_address(dst))
    {
        /* message send to me */
        /* check rpl */
        meshx_rpl_t rpl = {.src = src, .seq = seq};
        if (meshx_rpl_exists(rpl))
        {
            /* replay attaction happened */
            MESHX_WARN("replay attack happened!");
            return -MESHX_ERR_ALREADY;
        }

        int32_t ret = meshx_rpl_update(rpl);
        if (MESHX_SUCCESS != ret)
        {
            MESHX_ERROR("update rpl list failed: %d", ret);
            return ret;
        }
    }
    else
    {
        /* message need to realy */
        /* check nmc */
        meshx_nmc_t nmc = {.src = src, .dst = dst, .seq = seq};
        if (meshx_nmc_exists(nmc))
        {
            /* message already cached, ignore */
            MESHX_INFO("message already cached, igonre!");
            return -MESHX_ERR_ALREADY;
        }

        meshx_nmc_add(nmc);
    }

    /* decrypt transport layer data */
    uint8_t net_mic_len = net_pdu.net_metadata.ctl ? 8 : 4;
    uint8_t trans_pdu_len = len - sizeof(meshx_network_metadata_t) - net_mic_len;
    meshx_network_decrypt(&net_pdu, trans_pdu_len, iv_index, pnet_key);
    MESHX_DEBUG("decrypt net pdu:");
    MESHX_DUMP_DEBUG(&net_pdu, len - net_mic_len);


    /* send data to lower transport lower */
    return MESHX_SUCCESS;
}

int32_t meshx_network_send(meshx_network_if_t network_if,
                           const uint8_t *ptrans_pdu, uint8_t trans_pdu_len,
                           const meshx_msg_ctx_t *pmsg_ctx)
{
    if (NULL == network_if)
    {
        MESHX_ERROR("network interface is NULL");
        return -MESHX_ERR_INVAL;
    }

    if (trans_pdu_len > MESHX_NETWORK_TRANS_PDU_MAX_LEN)
    {
        return -MESHX_ERR_LENGTH;
    }

    meshx_bearer_t bearer = meshx_network_if_get_bearer(network_if);
    if (NULL == bearer)
    {
        MESHX_ERROR("network interface(0x%08x) hasn't connected to any bearer", network_if);
        return -MESHX_ERR_CONNECT;
    }

    /* filter data */
    meshx_network_if_output_filter_data_t filter_data = {.src_addr = meshx_node_address_get(), .dst_addr = pmsg_ctx->dst};
    if (!meshx_network_if_output_filter(network_if, &filter_data))
    {
        MESHX_INFO("data has been filtered!");
        return -MESHX_ERR_FILTER;
    }

    if (pmsg_ctx->ctl)
    {
        if (trans_pdu_len > MESHX_NETWORK_TRANS_PDU_MAX_LEN - 8)
        {
            MESHX_ERROR("invalid trans pdu length: %d-%d", pmsg_ctx->ctl, trans_pdu_len);
            return -MESHX_ERR_LENGTH;
        }
    }
    else
    {
        if (trans_pdu_len > MESHX_NETWORK_TRANS_PDU_MAX_LEN - 4)
        {
            MESHX_ERROR("invalid trans pdu length: %d-%d", pmsg_ctx->ctl, trans_pdu_len);
            return -MESHX_ERR_LENGTH;
        }
    }

    uint16_t src = meshx_node_address_get();
    uint32_t iv_index = meshx_iv_index_get();
    meshx_network_pdu_t net_pdu = {0};
    uint32_t seq = meshx_seq_get();
    net_pdu.net_metadata.ivi = (iv_index & 0x01);
    net_pdu.net_metadata.nid = pmsg_ctx->pnet_key->nid;
    net_pdu.net_metadata.ctl = pmsg_ctx->ctl;
    net_pdu.net_metadata.ttl = pmsg_ctx->ttl;
    net_pdu.net_metadata.seq[0] = seq >> 16;
    net_pdu.net_metadata.seq[1] = seq >> 8;
    net_pdu.net_metadata.seq[2] = seq;
    net_pdu.net_metadata.src = MESHX_HOST_TO_BE16(src);
    net_pdu.net_metadata.dst = MESHX_HOST_TO_BE16(pmsg_ctx->dst);
    memcpy(net_pdu.pdu, ptrans_pdu, trans_pdu_len);
    uint8_t net_mic_len = pmsg_ctx->ctl ? 8 : 4;
    uint8_t net_pdu_len = sizeof(meshx_network_metadata_t) + trans_pdu_len + net_mic_len;
    MESHX_DEBUG("origin network pdu:");
    MESHX_DUMP_DEBUG(&net_pdu, net_pdu_len);

    /* encrypt dst field and trans pdu */
    meshx_network_encrypt(&net_pdu, trans_pdu_len, iv_index, pmsg_ctx->pnet_key);

    /* obfuscation ctl, ttl, seq, src fields */
    meshx_network_obfuscation(&net_pdu, iv_index, pmsg_ctx->pnet_key);

    MESHX_DEBUG("encrypt and obsfucation net pdu:");
    MESHX_DUMP_DEBUG(&net_pdu, net_pdu_len);

    /* send data to bearer */
    uint8_t pkt_type;
    if (MESHX_BEARER_TYPE_ADV == bearer->type)
    {
        pkt_type = MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG;
    }
    else
    {
        pkt_type = MESHX_BEARER_GATT_PKT_TYPE_NETWORK;
    }

    return meshx_bearer_send(bearer, pkt_type, (const uint8_t *)&net_pdu, net_pdu_len);
}
