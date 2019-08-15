/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_LOWER_TRANSPORT"
#include "meshx_lower_transport.h"
#include "meshx_errno.h"

/* access message netmic is fixed to 32bit */

/* unsegment access message TransMIC size is fixed to 32bit */
typedef struct
{
    uint8_t aid : 6;
    uint8_t akf : 1;
    uint8_t seg : 1;
    uint8_t pdu[15];
    /* data */
} __PACKED meshx_lower_trans_unseg_access_pdu_t;

/* segmented access message TransMIC size is according to szmic field */
typedef struct
{
    uint8_t aid : 6;
    uint8_t akf : 1;
    uint8_t seg : 1;
    union
    {
        struct
        {
            uint32_t segn : 5;
            uint32_t sego : 5;
            uint32_t seq_zero : 13;
            uint32_t szmic : 1;
        };
        uint8_t seg_misc[3]; /* need to change to big endian */
    };
    uint8_t pdu[12];
} __PACKED meshx_lower_trans_seg_access_pdu_t;

/* control message netmic is fixed to 64bit */
typedef struct
{
    uint8_t opcode : 7;
    uint8_t seg : 1;
    uint8_t pdu[11];
} __PACKED meshx_lower_trans_unseg_ctl_pdu_t;

typedef struct
{
    uint8_t opcode : 7;
    uint8_t seg : 1;
    union
    {
        struct
        {
            uint32_t segn : 5;
            uint32_t sego : 5;
            uint32_t seq_zero : 13;
            uint32_t rfu : 1;
        };
        uint8_t seg_misc[3]; /* need to chang to big endian */
    };
    uint8_t pdu[8];
} __PACKED meshx_lower_trans_seg_ctl_pdu_t;


int32_t meshx_lower_transport_init(void)
{
    return MESHX_SUCCESS;
}

int32_t meshx_lower_transport_send(meshx_network_if_t network_if, const uint8_t *pupper_trans_pdu,
                                   uint8_t len, meshx_msg_ctx_t *pmsg_ctx)
{
    if (pmsg_ctx->ctl)
    {
        /* control message */
        if (pmsg_ctx->force_seg)
        {
            /* force segment */
        }
        else
        {
            /* code */
        }

    }
    else
    {
        /* access message */
        if (pmsg_ctx->force_seg)
        {
            /* force segment */
        }
        else
        {
        }

    }

    meshx_network_send(network_if, pupper_trans_pdu, len, pmsg_ctx);
    return MESHX_SUCCESS;
}
