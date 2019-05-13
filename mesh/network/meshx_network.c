/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MESHX_NETWORK"
#include "meshx_trace.h"
#include "meshx_network.h"
#include "meshx_errno.h"
#include "meshx_config.h"
#include "meshx_bearer_internal.h"
#include "meshx_network_internal.h"

//static uint32_t seq = 0;

int32_t meshx_network_init(void)
{
    meshx_network_if_init();
    return MESHX_SUCCESS;
}

int32_t meshx_network_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    meshx_network_if_t network_if = meshx_network_if_get(bearer);
    if (MESHX_NETWORK_IF_TYPE_INVALID == network_if.type)
    {
        MESHX_WARN("bearer(%d) is not connected to network  interface!", bearer.bearer);
        return -MESHX_ERR_BEARER_CONNECT;
    }

    /* decrypt data */
    meshx_network_pdu_t net_pdu = {0};
    uint8_t pdu_len = 0;

    /* filter data */
    if (!meshx_network_if_input_filter(network_if, &net_pdu, pdu_len))
    {
        return -MESHX_ERR_FILTER;
    }


    /* send data to lower transport lower */
    return MESHX_SUCCESS;
}

int32_t meshx_network_send(meshx_network_if_t network_if, const meshx_network_pdu_t *ppdu,
                           uint8_t pdu_len)
{
    if (pdu_len > MESHX_NETWORK_PDU_MAX_LEN)
    {
        return -MESHX_ERR_LENGTH;
    }

    meshx_bearer_t bearer = meshx_network_if_get_bearer(network_if);
    if (MESHX_BEARER_TYPE_INVALID != bearer.type)
    {
        return -MESHX_ERR_BEARER_CONNECT;
    }

    /* filter data */
    if (!meshx_network_if_output_filter(network_if, ppdu, pdu_len))
    {
        return -MESHX_ERR_FILTER;
    }

    /* encrypt data */

    /* send data to bearer */
    uint8_t pkt_type;
    if (MESHX_BEARER_TYPE_ADV == bearer.type)
    {
        pkt_type = MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG;
    }
    else
    {
        pkt_type = MESHX_BEARER_GATT_PKT_TYPE_NETWORK;
    }

    return meshx_bearer_send(bearer, pkt_type, (const uint8_t *)ppdu, pdu_len);
}
