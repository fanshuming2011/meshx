/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_NET"
#include "meshx_trace.h"
#include "meshx_net.h"
#include "meshx_errno.h"
#include "meshx_config.h"
#include "meshx_bearer_internal.h"
#include "meshx_net_internal.h"
#include "meshx_seq.h"
#include "meshx_iv_index.h"
#include "meshx_node.h"
#include "meshx_node_internal.h"
#include "meshx_security.h"
#include "meshx_endianness.h"
#include "meshx_nmc.h"
#include "meshx_rpl.h"
#include "meshx_key.h"
#include "meshx_lower_trans.h"

#define MESHX_NET_TRANS_PDU_MAX_LEN         20
#define MESHX_NET_ENCRYPT_OFFSET            7
#define MESHX_NET_OBFUSCATION_OFFSET        1
#define MESHX_NET_OBFUSCATION_SIZE          6

typedef struct
{
    uint8_t nid : 7;
    uint8_t ivi : 1;
    uint8_t ttl : 7;
    uint8_t ctl : 1;
    uint8_t seq[3];
    uint16_t src;
    uint16_t dst;
} __PACKED meshx_net_metadata_t;

typedef struct
{
    meshx_net_metadata_t net_metadata;
    uint8_t pdu[MESHX_NET_TRANS_PDU_MAX_LEN]; /* netmic is 32bit if ctl is 0, netmic is 64bit if ctl is 1*/
} __PACKED meshx_net_pdu_t;


int32_t meshx_net_init(void)
{
    meshx_net_iface_init();
    return MESHX_SUCCESS;
}

static void meshx_net_encrypt(meshx_net_pdu_t *pnet_pdu, uint8_t trans_pdu_len,
                              uint32_t iv_index, const meshx_net_key_t *pnet_key)
{
    meshx_net_nonce_t net_nonce;
    net_nonce.nonce_type = MESHX_NONCE_TYPE_NET;
    net_nonce.ctl = pnet_pdu->net_metadata.ctl;
    net_nonce.ttl = pnet_pdu->net_metadata.ttl;
    net_nonce.seq[0] = pnet_pdu->net_metadata.seq[0];
    net_nonce.seq[1] = pnet_pdu->net_metadata.seq[1];
    net_nonce.seq[2] = pnet_pdu->net_metadata.seq[2];
    net_nonce.src = pnet_pdu->net_metadata.src;
    net_nonce.pad = 0;
    net_nonce.iv_index = MESHX_HOST_TO_BE32(iv_index);
    MESHX_DEBUG("net nonce:");
    MESHX_DUMP_DEBUG(&net_nonce, sizeof(meshx_net_nonce_t));

    uint8_t net_mic_len = pnet_pdu->net_metadata.ctl ? 8 : 4;
    /* encrypt data */
    meshx_aes_ccm_encrypt(pnet_key->encryption_key, (const uint8_t *)&net_nonce,
                          sizeof(meshx_net_nonce_t),
                          NULL, 0, (const uint8_t *)pnet_pdu + MESHX_NET_ENCRYPT_OFFSET,
                          sizeof(pnet_pdu->net_metadata.dst) + trans_pdu_len,
                          (uint8_t *)pnet_pdu + MESHX_NET_ENCRYPT_OFFSET, pnet_pdu->pdu + trans_pdu_len, net_mic_len);
    MESHX_DEBUG("encrypt and auth trans pdu:");
    MESHX_DUMP_DEBUG((uint8_t *)pnet_pdu + MESHX_NET_ENCRYPT_OFFSET,
                     sizeof(pnet_pdu->net_metadata.dst) + trans_pdu_len + net_mic_len);
}

static void meshx_net_obfuscation(meshx_net_pdu_t *pnet_pdu, uint32_t iv_index,
                                  const meshx_net_key_t *pnet_key)
{
    iv_index = MESHX_HOST_TO_BE32(iv_index);
    uint8_t privacy_plaintext[16];
    memset(privacy_plaintext, 0, 5);
    memcpy(privacy_plaintext + 5, &iv_index, 4);
    memcpy(privacy_plaintext + 9, (const uint8_t *)pnet_pdu + MESHX_NET_ENCRYPT_OFFSET, 7);
    MESHX_DEBUG("privacy plaintext:");
    MESHX_DUMP_DEBUG(privacy_plaintext, 16);
    meshx_e(privacy_plaintext, pnet_key->privacy_key, privacy_plaintext);
    MESHX_DEBUG("PECB:");
    MESHX_DUMP_DEBUG(privacy_plaintext, 16);
    uint8_t *pdata = (uint8_t *)pnet_pdu + MESHX_NET_OBFUSCATION_OFFSET;
    for (uint8_t i = 0; i < MESHX_NET_OBFUSCATION_SIZE; ++i)
    {
        *pdata = *pdata ^ privacy_plaintext[i];
        pdata ++;
    }
    MESHX_DEBUG("obfuscation net header:");
    MESHX_DUMP_DEBUG((uint8_t *)pnet_pdu + MESHX_NET_OBFUSCATION_OFFSET,
                     MESHX_NET_OBFUSCATION_SIZE);
}

static int32_t meshx_net_decrypt(meshx_net_pdu_t *pnet_pdu, uint8_t trans_pdu_len,
                                 uint32_t iv_index, const meshx_net_key_t *pnet_key)
{
    meshx_net_nonce_t net_nonce;
    net_nonce.nonce_type = MESHX_NONCE_TYPE_NET;
    net_nonce.ctl = pnet_pdu->net_metadata.ctl;
    net_nonce.ttl = pnet_pdu->net_metadata.ttl;
    net_nonce.seq[0] = pnet_pdu->net_metadata.seq[0];
    net_nonce.seq[1] = pnet_pdu->net_metadata.seq[1];
    net_nonce.seq[2] = pnet_pdu->net_metadata.seq[2];
    net_nonce.src = pnet_pdu->net_metadata.src;
    net_nonce.pad = 0;
    net_nonce.iv_index = MESHX_HOST_TO_BE32(iv_index);
    MESHX_DEBUG("net nonce:");
    MESHX_DUMP_DEBUG(&net_nonce, sizeof(meshx_net_nonce_t));

    uint8_t net_mic_len = pnet_pdu->net_metadata.ctl ? 8 : 4;
    /* decrypt data */
    int32_t ret = meshx_aes_ccm_decrypt(pnet_key->encryption_key, (const uint8_t *)&net_nonce,
                                        sizeof(meshx_net_nonce_t),
                                        NULL, 0, (const uint8_t *)pnet_pdu + MESHX_NET_ENCRYPT_OFFSET,
                                        sizeof(pnet_pdu->net_metadata.dst) + trans_pdu_len,
                                        (uint8_t *)pnet_pdu + MESHX_NET_ENCRYPT_OFFSET, pnet_pdu->pdu + trans_pdu_len, net_mic_len);
    if (MESHX_SUCCESS == ret)
    {
        MESHX_DEBUG("dencrypt trans pdu:");
        MESHX_DUMP_DEBUG((uint8_t *)pnet_pdu + MESHX_NET_ENCRYPT_OFFSET,
                         sizeof(pnet_pdu->net_metadata.dst) + trans_pdu_len);
    }
    else
    {
        MESHX_ERROR("decrypt net pdu failed!");
    }

    return ret;
}

int32_t meshx_net_receive(meshx_net_iface_t net_iface, const uint8_t *pdata, uint8_t len)
{
    MESHX_DEBUG("receive net data:");
    MESHX_DUMP_DEBUG(pdata, len);

#if MESHX_REDUNDANCY_CHECK
    if (NULL == net_iface)
    {
        MESHX_ERROR("net interface is NULL");
        return -MESHX_ERR_INVAL;
    }
#endif

    /* filter data */
    /* TODO: add input filter data, use adv information? */
    meshx_net_iface_ifilter_data_t filter_data = {};
    if (!meshx_net_iface_ifilter(net_iface, &filter_data))
    {
        MESHX_WARN("net data has been filtered!");
        return -MESHX_ERR_FILTER;
    }

    /* copy data */
    uint32_t iv_index = meshx_iv_index_get();
    meshx_net_pdu_t net_pdu = *(const meshx_net_pdu_t *)pdata;

    /* check ivi to choose correct iv index */
    if ((iv_index & 0x01) !=  net_pdu.net_metadata.ivi)
    {
        iv_index = iv_index - 1;
    }

    /* get net key */
    const meshx_net_key_t *pnet_key = NULL;
    meshx_net_key_traverse_start(&pnet_key, net_pdu.net_metadata.nid);
    if (NULL == pnet_key)
    {
        MESHX_DEBUG("no key's nid is 0x%x", net_pdu.net_metadata.nid);
        return -MESHX_ERR_KEY;
    }

    int32_t ret = MESHX_SUCCESS;
    uint8_t net_mic_len, trans_pdu_len;
    while (NULL != pnet_key)
    {
        /* restore header */
        meshx_net_obfuscation(&net_pdu, iv_index, pnet_key);

        /* decrypt transport layer data */
        net_mic_len = net_pdu.net_metadata.ctl ? 8 : 4;
        trans_pdu_len = len - sizeof(meshx_net_metadata_t) - net_mic_len;
        ret = meshx_net_decrypt(&net_pdu, trans_pdu_len, iv_index, pnet_key);
        if (MESHX_SUCCESS == ret)
        {
            break;
        }
        else
        {
            meshx_net_key_traverse_continue(&pnet_key, net_pdu.net_metadata.nid);
        }
    }

    if (NULL == pnet_key)
    {
        MESHX_ERROR("can't decrypt pdu with net key that nid is 0x%x", net_pdu.net_metadata.nid);
        return -MESHX_ERR_KEY;
    }


    /* TODO: check nmc, rpl and relay or loopback interface */
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

    /* check nmc */
    meshx_nmc_t nmc = {.src = src, .seq = seq};
    if (!meshx_nmc_check(nmc))
    {
        /* message already cached, ignore */
        return -MESHX_ERR_FAIL;
    }

    meshx_nmc_add(nmc);

    MESHX_INFO("receive network pdu: ctl %d, ttl %d, src 0x%04x, dst 0x%04x, seq 0x%06x, iv_index 0x%08x",
               net_pdu.net_metadata.ctl,
               net_pdu.net_metadata.ttl, src, dst, seq, iv_index);
    MESHX_DUMP_INFO(net_pdu.pdu, trans_pdu_len);


    if (meshx_node_is_my_address(dst))
    {
        /* message send to me */
        /* send data to lower transport lower */
        meshx_msg_ctx_t msg_ctx;
        memset(&msg_ctx, 0, sizeof(meshx_msg_ctx_t));
        msg_ctx.ctl = net_pdu.net_metadata.ctl;
        msg_ctx.ttl = net_pdu.net_metadata.ttl;
        msg_ctx.src = src;
        msg_ctx.dst = dst;
        msg_ctx.iv_index = iv_index;
        msg_ctx.seq = seq;
        msg_ctx.pnet_key = pnet_key;
        msg_ctx.net_iface = net_iface;
        ret = meshx_lower_trans_receive(net_pdu.pdu, trans_pdu_len, &msg_ctx);
    }
    else
    {
        MESHX_INFO("message need to relay");
        /* message need to relay */
        /* TODO: check ttl and relay*/
    }

    return ret;
}

static int32_t meshx_net_loopback(const uint8_t *pdata, uint8_t len,
                                  const meshx_msg_ctx_t *pmsg_tx_ctx)
{
    /* send on loopback interface */
    meshx_net_iface_t loopback_iface = meshx_net_iface_get(NULL);
    if (NULL == loopback_iface)
    {
        MESHX_ERROR("no loopback interface exist!");
    }
    /* filter data */
    meshx_net_iface_ofilter_data_t filter_data = {.src_addr = pmsg_tx_ctx->src, .dst_addr = pmsg_tx_ctx->dst};
    if (!meshx_net_iface_ofilter(loopback_iface, &filter_data))
    {
        MESHX_INFO("network data has been filtered");
        return -MESHX_ERR_FILTER;
    }

    return meshx_net_receive(loopback_iface, pdata, len);
}

static int32_t meshx_net_send_to_bearer(const uint8_t *pdata, uint8_t len,
                                        const meshx_msg_ctx_t *pmsg_tx_ctx, meshx_net_iface_info_t *piface)
{
    /* filter data */
    meshx_net_iface_ofilter_data_t filter_data = {.src_addr = pmsg_tx_ctx->src, .dst_addr = pmsg_tx_ctx->dst};
    if (!meshx_net_iface_ofilter(piface, &filter_data))
    {
        MESHX_INFO("network data has been filtered");
        return -MESHX_ERR_FILTER;
    }

    meshx_bearer_t bearer = piface->bearer;
    if (NULL == bearer)
    {
        MESHX_WARN("net interface(0x%08x) hasn't been connected to any bearer", piface);
        return -MESHX_ERR_CONNECT;
    }

    /* send data out */
    uint8_t pkt_type = 0;
    if (MESHX_BEARER_TYPE_ADV == bearer->type)
    {
        pkt_type = MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG;
    }
    else
    {
        pkt_type = MESHX_BEARER_GATT_PKT_TYPE_NET;
    }

    return meshx_bearer_send(bearer, pkt_type, pdata, len);
}

int32_t meshx_net_send(const uint8_t *ptrans_pdu, uint8_t trans_pdu_len,
                       const meshx_msg_ctx_t *pmsg_tx_ctx)
{
#if MESHX_REDUNDANCY_CHECK
    if (trans_pdu_len > MESHX_NET_TRANS_PDU_MAX_LEN)
    {
        return -MESHX_ERR_LENGTH;
    }

    if (pmsg_tx_ctx->ctl)
    {
        if (trans_pdu_len > MESHX_NET_TRANS_PDU_MAX_LEN - 8)
        {
            MESHX_ERROR("invalid trans pdu length for control message: %d-%d", pmsg_tx_ctx->ctl, trans_pdu_len);
            return -MESHX_ERR_LENGTH;
        }
    }
    else
    {
        if (trans_pdu_len > MESHX_NET_TRANS_PDU_MAX_LEN - 4)
        {
            MESHX_ERROR("invalid trans pdu length for access message: %d-%d", pmsg_tx_ctx->ctl, trans_pdu_len);
            return -MESHX_ERR_LENGTH;
        }
    }
#endif

    uint32_t seq = pmsg_tx_ctx->seq;
    meshx_net_pdu_t net_pdu = {0};
    net_pdu.net_metadata.ivi = (pmsg_tx_ctx->iv_index & 0x01);
    net_pdu.net_metadata.nid = pmsg_tx_ctx->pnet_key->nid;
    net_pdu.net_metadata.ctl = pmsg_tx_ctx->ctl;
    net_pdu.net_metadata.ttl = pmsg_tx_ctx->ttl;
    net_pdu.net_metadata.seq[0] = seq >> 16;
    net_pdu.net_metadata.seq[1] = seq >> 8;
    net_pdu.net_metadata.seq[2] = seq;
    net_pdu.net_metadata.src = MESHX_HOST_TO_BE16(pmsg_tx_ctx->src);
    net_pdu.net_metadata.dst = MESHX_HOST_TO_BE16(pmsg_tx_ctx->dst);
    memcpy(net_pdu.pdu, ptrans_pdu, trans_pdu_len);
    uint8_t net_mic_len = pmsg_tx_ctx->ctl ? 8 : 4;
    uint8_t net_pdu_len = sizeof(meshx_net_metadata_t) + trans_pdu_len + net_mic_len;
    MESHX_DEBUG("origin net pdu:");
    MESHX_DUMP_DEBUG(&net_pdu, net_pdu_len);

    /* encrypt dst field and trans pdu */
    meshx_net_encrypt(&net_pdu, trans_pdu_len, pmsg_tx_ctx->iv_index, pmsg_tx_ctx->pnet_key);

    /* obfuscation ctl, ttl, seq, src fields */
    meshx_net_obfuscation(&net_pdu, pmsg_tx_ctx->iv_index, pmsg_tx_ctx->pnet_key);

    MESHX_DEBUG("encrypt and obsfucation net pdu:");
    MESHX_DUMP_DEBUG(&net_pdu, net_pdu_len);

    int32_t ret = MESHX_SUCCESS;
    if (NULL == pmsg_tx_ctx->net_iface)
    {
        /* TODO: check wheter is lpn addr? */
        if (meshx_node_is_my_address(pmsg_tx_ctx->dst))
        {
            ret = meshx_net_loopback((const uint8_t *)&net_pdu, net_pdu_len, pmsg_tx_ctx);
        }
        else
        {
            /* code */
            meshx_list_t *pnode = NULL;
            meshx_net_iface_info_t *piface;
            meshx_list_foreach(pnode, &meshx_net_iface_list)
            {
                piface = MESHX_CONTAINER_OF(pnode, meshx_net_iface_info_t, node);
                if (MESHX_NET_IFACE_TYPE_LOOPBACK == piface->type)
                {
                    continue;
                }
                meshx_net_send_to_bearer((const uint8_t *)&net_pdu, net_pdu_len, pmsg_tx_ctx, piface);
            }
        }
    }
    else
    {
        /* send on specified network interface */
        meshx_net_iface_info_t *piface = (meshx_net_iface_info_t *)pmsg_tx_ctx->net_iface;
        if (MESHX_NET_IFACE_TYPE_LOOPBACK == piface->type)
        {
            ret = meshx_net_loopback((const uint8_t *)&net_pdu, net_pdu_len, pmsg_tx_ctx);
        }
        else
        {
            ret = meshx_net_send_to_bearer((const uint8_t *)&net_pdu, net_pdu_len, pmsg_tx_ctx, piface);
        }
    }

    return ret;
}
