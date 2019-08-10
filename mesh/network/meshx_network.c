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

//static uint32_t seq = 0;

typedef struct
{
    uint8_t ivi : 1;
    uint8_t nid : 7;
    uint8_t ctl : 1;
    uint8_t ttl : 7;
    uint8_t seq[3];
    uint16_t src;
    uint16_t dst;
} __PACKED meshx_network_metadata_t;

typedef struct
{
    meshx_network_metadata_t net_metadata;
    uint8_t pdu[MESHX_NETWORK_PDU_MAX_LEN -
                                          9]; /* netmic is 32bit if ctl is 0, nwtmic is 64bit if ctl is 1*/
} __PACKED meshx_network_pdu_t;



int32_t meshx_network_init(void)
{
    meshx_network_if_init();
    return MESHX_SUCCESS;
}

int32_t meshx_network_receive(meshx_network_if_t network_if, const uint8_t *pdata, uint8_t len)
{
    if (NULL == network_if)
    {
        MESHX_ERROR("network interface is NULL");
        return -MESHX_ERR_INVAL;
    }

    /* decrypt data */
    meshx_network_pdu_t net_pdu = {0};

    /* filter data */
    meshx_network_if_filter_data_t filter_data = {.src_addr = net_pdu.net_metadata.src, .dst_addr = net_pdu.net_metadata.dst};
    if (!meshx_network_if_input_filter(network_if, &filter_data))
    {
        return -MESHX_ERR_FILTER;
    }


    /* send data to lower transport lower */
    return MESHX_SUCCESS;
}

int32_t meshx_network_send(meshx_network_if_t network_if,
                           const uint8_t *ppdu, uint8_t pdu_len,
                           const meshx_msg_ctx_t *pmsg_ctx)
{
    if (NULL == network_if)
    {
        MESHX_ERROR("network interface is NULL");
        return -MESHX_ERR_INVAL;
    }

    if (pdu_len > MESHX_NETWORK_PDU_MAX_LEN)
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
    meshx_network_if_filter_data_t filter_data = {.src_addr = meshx_node_address_get(), .dst_addr = pmsg_ctx->dst};
    if (!meshx_network_if_output_filter(network_if, &filter_data))
    {
        return -MESHX_ERR_FILTER;
    }

    if (pmsg_ctx->ctl)
    {
        if (pdu_len > MESHX_NETWORK_PDU_MAX_LEN - 9 - 8)
        {
            return -MESHX_ERR_LENGTH;
        }
    }
    else
    {
        if (pdu_len > MESHX_NETWORK_PDU_MAX_LEN - 9 - 4)
        {
            return -MESHX_ERR_LENGTH;
        }
    }

#if 0
    meshx_network_pdu_t net_pdu = {0};
    uint32_t seq = meshx_seq_get();
    net_pdu.net_metadata.ivi = (meshx_iv_index_get() & 0x01);
    net_pdu.net_metadata.nid = pmsg_ctx->pnet_key->nid;
    net_pdu.net_metadata.ctl = pmsg_ctx->ctl;
    net_pdu.net_metadata.ttl = pmsg_ctx->ttl;
    net_pdu.net_metadata.seq[0] = seq;
    net_pdu.net_metadata.seq[1] = seq >> 8;
    net_pdu.net_metadata.seq[2] = seq >> 16;
    net_pdu.net_metadata.src = meshx_node_address_get();
    net_pdu.net_metadata.dst = pmsg_ctx->dst;
    memcpy(net_pdu.pdu, ppdu, pdu_len);
    uint8_t data_len = sizeof(net_pdu.net_metadata) + pdu_len;
    uint8_t net_mic_len = pmsg_ctx->ctl ? 8 : 4;
    uint8_t net_mic[8];

    /* encrypt data */
    meshx_aes_ccm_encrypt(pmsg_ctx->pnet_key->net_key, const uint8_t key[16], const uint8_t *piv,
                          uint32_t iv_len,
                          const uint8_t *padd, uint32_t add_len,
                          const uint8_t *pinput, uint32_t length, uint8_t *poutput,
                          uint8_t *ptag, uint32_t tag_len)
#endif

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

    return meshx_bearer_send(bearer, pkt_type, (const uint8_t *)ppdu, pdu_len);
}
