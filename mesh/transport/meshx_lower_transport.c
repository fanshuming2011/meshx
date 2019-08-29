/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_LOWER_TRANSPORT"
#include "meshx_lower_transport.h"
#include "meshx_errno.h"
#include "meshx_list.h"
#include "meshx_node_internal.h"
#include "meshx_mem.h"
#include "meshx_timer.h"
#include "meshx_trace.h"
#include "meshx_assert.h"
#include "meshx_misc.h"
#include "meshx_async_internal.h"

/**
 *  NOTE: only one segment message can send to the same destination once a time,
 *        no ack from group or virtual address, ack from unicast address, so recommand
 *        to send multiple times when destination is group or virtual address
 **/


#define MESHX_LOWER_TRANS_UNSEG_ACCESS_MAX_PDU_SIZE             15
#define MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE               12
#define MESHX_LOWER_TRANS_UNSEG_CTL_MAX_PDU_SIZE                11
#define MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE                  8

#define MESHX_LOWER_TRANS_MAX_SEG_SIZE                          0x1F

#define MESHX_LOWER_TRANS_RETRY_PERIOD                          300 /* TODO: can configure */

#define MESHX_LOWER_TRANS_MAX_RETRY_TIMES                       3 /* TODO: can configure */

/**/
typedef struct
{
    meshx_network_if_t network_if;
    meshx_msg_ctx_t msg_ctx;
    uint8_t *ppdu;
    uint16_t pdu_len;
    meshx_timer_t retry_timer;
    uint8_t retry_times;
    meshx_list_t node;
} meshx_lower_trans_task_t;

static meshx_list_t meshx_lower_trans_task_idle;
static meshx_list_t meshx_lower_trans_task_active;


/* access message NetMIC is fixed to 32bit */
typedef struct
{
    uint8_t aid : 6;
    uint8_t akf : 1;
    uint8_t seg : 1;
} __PACKED meshx_lower_trans_access_pdu_metadata_t;

/* unsegment access message TransMIC size is fixed to 32bit */
typedef struct
{
    meshx_lower_trans_access_pdu_metadata_t metadata;
    uint8_t pdu[MESHX_LOWER_TRANS_UNSEG_ACCESS_MAX_PDU_SIZE];
    /* data */
} __PACKED meshx_lower_trans_unseg_access_pdu_t;

/* segmented access message TransMIC size is according to szmic field */
typedef struct
{
    meshx_lower_trans_access_pdu_metadata_t metadata;
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
    uint8_t pdu[MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE];
} __PACKED meshx_lower_trans_seg_access_pdu_t;

/* control message NetMIC is fixed to 64bit */
typedef struct
{
    uint8_t opcode : 7;
    uint8_t seg : 1;
    uint8_t pdu[MESHX_LOWER_TRANS_UNSEG_CTL_MAX_PDU_SIZE];
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
    uint8_t pdu[MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE];
} __PACKED meshx_lower_trans_seg_ctl_pdu_t;


int32_t meshx_lower_transport_init(void)
{
    meshx_list_init_head(&meshx_lower_trans_task_idle);
    meshx_list_init_head(&meshx_lower_trans_task_active);
    meshx_lower_trans_task_t *ptasks = meshx_malloc(meshx_node_params.config.trans_task_num * sizeof(
                                                        meshx_lower_trans_task_t));
    if (NULL == ptasks)
    {
        MESHX_ERROR("initialize lower transport failed: out of memory!");
        return -MESHX_ERR_MEM;
    }
    memset(ptasks, 0, meshx_node_params.config.trans_task_num * sizeof(meshx_lower_trans_task_t));
    for (uint8_t i = 0; i < meshx_node_params.config.trans_task_num ; ++i)
    {
        meshx_list_append(&meshx_lower_trans_task_idle, &ptasks[i].node);
    }

    return MESHX_SUCCESS;
}

static uint8_t meshx_lower_trans_random(void)
{
    uint32_t random = MESHX_ABS(meshx_rand());
    random %= 50;
    if (random < 20)
    {
        random += 20;
    }

    return random;
}

static void meshx_lower_trans_task_release(meshx_lower_trans_task_t *ptask)
{
    MESHX_DEBUG("release task: 0x%08x", ptask);
    MESHX_ASSERT(NULL != ptask);
    if (NULL != ptask->retry_timer)
    {
        meshx_timer_delete(ptask->retry_timer);
        ptask->retry_timer = NULL;
    }
    if (NULL != ptask->ppdu)
    {
        meshx_free(ptask->ppdu);
        ptask->ppdu = NULL;
    }
    meshx_list_append(&meshx_lower_trans_task_idle, &ptask->node);
}

static void meshx_lower_trans_timeout_handler(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

static void meshx_lower_trans_send_seg_msg(meshx_network_if_t network_if, const uint8_t *ppdu,
                                           uint16_t pdu_len, const meshx_msg_ctx_t *pmsg_ctx)
{
    MESHX_INFO("send seg message");
    uint8_t seg_num = (pdu_len + MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE - 1) /
                      MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
    /* send segment message for the first time */
    uint8_t seg_len;
    uint16_t data_offset = 0;
    meshx_lower_trans_seg_access_pdu_t pdu;
    pdu.metadata.aid = pmsg_ctx->aid;
    pdu.metadata.akf = pmsg_ctx->akf;
    pdu.metadata.seg = 1;
    pdu.szmic = pmsg_ctx->szmic;
    pdu.segn = seg_num - 1;
    for (uint8_t i = 0; i < seg_num; ++i)
    {
        pdu.seq_zero = pmsg_ctx->seq_zero;
        pdu.sego = i;
        seg_len = (seg_num == (i + 1)) ? (pdu_len - data_offset) :
                  MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
        memcpy(pdu.pdu, ppdu + data_offset, seg_len);
        seg_len += sizeof(meshx_lower_trans_access_pdu_metadata_t);
        meshx_network_send(network_if, (const uint8_t *)&pdu, seg_len, pmsg_ctx);
        data_offset += seg_len;
    }
}

static void meshx_lower_trans_handle_timeout(meshx_lower_trans_task_t *ptask)
{
    MESHX_INFO("retrans: %d!!!", ptask->retry_times);
    ptask->retry_times ++;
    if (ptask->retry_times > MESHX_LOWER_TRANS_MAX_RETRY_TIMES)
    {
        if (MESHX_ADDRESS_IS_UNICAST(ptask->msg_ctx.dst))
        {
            /* TODO: notify upper transport send failed, no ack from remote or some segment can't be received */
        }
        else
        {
            /* TODO: notify upper transport send finished */
        }
        meshx_lower_trans_task_release(ptask);
    }
    else
    {
        /* send segment message */
        meshx_lower_trans_send_seg_msg(ptask->network_if, ptask->ppdu, ptask->pdu_len, &ptask->msg_ctx);
    }
}

void meshx_lower_trans_async_handle_timeout(meshx_async_msg_t msg)
{
    meshx_lower_trans_handle_timeout(msg.pdata);
}

static meshx_lower_trans_task_t *meshx_lower_trans_task_request(uint16_t pdu_len)
{
    if (meshx_list_is_empty(&meshx_lower_trans_task_idle))
    {
        return NULL;
    }

    meshx_list_t *pnode = meshx_list_pop(&meshx_lower_trans_task_idle);
    MESHX_ASSERT(NULL != pnode);
    meshx_lower_trans_task_t *ptask =  MESHX_CONTAINER_OF(pnode, meshx_lower_trans_task_t, node);
    if (MESHX_SUCCESS != meshx_timer_create(&ptask->retry_timer, MESHX_TIMER_MODE_REPEATED,
                                            meshx_lower_trans_timeout_handler, ptask))
    {
        MESHX_ERROR("request lower transport task failed: out of resource!");
        return NULL;
    }
    ptask->ppdu = meshx_malloc(pdu_len);
    if (NULL == ptask->ppdu)
    {
        meshx_list_append(pnode, &meshx_lower_trans_task_idle);
        MESHX_ERROR("request lower transport task failed: out of memory!");
        return NULL;
    }

    ptask->retry_times = 0;

    return ptask;
}

#if 0
static bool meshx_lower_trans_is_sending(uint16_t dst)
{
    return TRUE;
}
#endif

int32_t meshx_lower_transport_send(meshx_network_if_t network_if, const uint8_t *pupper_trans_pdu,
                                   uint16_t pdu_len, meshx_msg_ctx_t *pmsg_ctx)
{
    if (0 == pdu_len)
    {
        MESHX_ERROR("no valid message!");
        return -MESHX_ERR_LENGTH;
    }

    if (!MESHX_ADDRESS_IS_VALID(pmsg_ctx->dst))
    {
        MESHX_ERROR("invlaid destination address: 0x%4x", pmsg_ctx->dst);
        return -MESHX_ERR_INVAL;
    }

    if (pmsg_ctx->ctl)
    {
        /* control message */
        if (pmsg_ctx->force_seg)
        {
            /* force segment */
        }
        else
        {
        }
    }
    else
    {
        /* access message */
        if (pmsg_ctx->force_seg || (pdu_len > MESHX_LOWER_TRANS_UNSEG_ACCESS_MAX_PDU_SIZE))
        {
            /* segment message */
            uint8_t seg_num = (pdu_len + MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE - 1) /
                              MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
            if (seg_num > MESHX_LOWER_TRANS_MAX_SEG_SIZE)
            {
                MESHX_ERROR("message length exceed maximum size: %d-%d", pdu_len,
                            MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE * MESHX_LOWER_TRANS_MAX_SEG_SIZE);
                return -MESHX_ERR_LENGTH;
            }

            /* store segment message for retransmit */
            meshx_lower_trans_task_t *ptask = meshx_lower_trans_task_request(pdu_len);
            if (NULL == ptask)
            {
                MESHX_ERROR("lower transport is busy now, try again later!");
                return -MESHX_ERR_BUSY;
            }
            ptask->network_if = network_if;
            ptask->msg_ctx = *pmsg_ctx;
            memcpy(ptask->ppdu, pupper_trans_pdu, pdu_len);
            ptask->pdu_len = pdu_len;

            /* check destination address */
            if (MESHX_ADDRESS_IS_UNICAST(pmsg_ctx->dst))
            {
                /* set timer to retrans delay */
                meshx_timer_start(ptask->retry_timer, MESHX_LOWER_TRANS_RETRY_PERIOD);
            }
            else
            {
                /* set timer to small random delay */
                meshx_timer_start(ptask->retry_timer, meshx_lower_trans_random());
            }

            /* send segment message for the first time */
            meshx_lower_trans_send_seg_msg(network_if, pupper_trans_pdu, pdu_len, pmsg_ctx);
        }
        else
        {
            /* unsegment message */
            meshx_lower_trans_unseg_access_pdu_t pdu;
            pdu.metadata.aid = pmsg_ctx->aid;
            pdu.metadata.akf = pmsg_ctx->akf;
            pdu.metadata.seg = 0;
            memcpy(pdu.pdu, pupper_trans_pdu, pdu_len);
            meshx_network_send(network_if, (const uint8_t *)&pdu,
                               sizeof(meshx_lower_trans_access_pdu_metadata_t) + pdu_len, pmsg_ctx);
        }
    }

    return MESHX_SUCCESS;
}
