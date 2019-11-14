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
    }

    return MESHX_SUCCESS;
}

int32_t meshx_proxy_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint16_t len)
{
    return MESHX_SUCCESS;
}

