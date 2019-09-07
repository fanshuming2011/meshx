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

#define MESHX_LOWER_TRANS_TX_RETRY_BASE                         200
#define MESHX_LOWER_TRANS_TX_RETRY_TTL_FACTOR                   50 /* 50 * ttl */

#define MESHX_LOWER_TRANS_MAX_RETRY_TIMES                       0 /* TODO: can configure */

#define MESHX_LOWER_TRANS_MAX_SEQ_DELTA                         0x2000

#define MESHX_LOWER_TRANS_INCOMPLETE_TIMEOUT                    100000 /* ms */
#define MESHX_LOWER_TRANS_RX_ACK_BASE                           150
#define MESHX_LOWER_TRANS_RX_ACK_TTL_FACTOR                     50 /* 50 * ttl */

/* lower transport tx task */
typedef struct
{
    meshx_network_if_t network_if;
    meshx_msg_tx_ctx_t msg_tx_ctx;
    uint8_t *ppdu;
    uint16_t pdu_len;
    meshx_timer_t retry_timer;
    uint8_t retry_times;
    meshx_list_t node;
} meshx_lower_trans_tx_task_t;

/* tx active task maybe exist same dst */
static meshx_list_t meshx_lower_trans_tx_task_idle;
static meshx_list_t meshx_lower_trans_tx_task_active;
static meshx_list_t meshx_lower_trans_tx_task_pending;

/* lower transport rx task */
typedef struct
{
    meshx_network_if_t network_if;
    meshx_msg_rx_ctx_t msg_rx_ctx;
    uint8_t *ppdu;
    uint16_t pdu_len;
    uint16_t max_pdu_len;
    uint32_t not_received_seg;
    meshx_timer_t ack_timer;
    meshx_timer_t incomplete_timer;
    meshx_list_t node;
} meshx_lower_trans_rx_task_t;

/* rx active task shall not exist same dst */
static meshx_list_t meshx_lower_trans_rx_task_idle;
static meshx_list_t meshx_lower_trans_rx_task_active;

typedef struct
{
    uint16_t src;
    uint32_t seq_origin;
    uint32_t iv_index;
    meshx_timer_t cancel_timer;
    meshx_list_t node;
} meshx_lower_trans_received_t;

/* rx active task shall not exist same dst */
//static meshx_list_t meshx_lower_trans_received_idle;
//static meshx_list_t meshx_lower_trans_received_active;




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
typedef union
{
    struct
    {
        uint32_t segn : 5;
        uint32_t sego : 5;
        uint32_t seq_zero : 13;
        uint32_t szmic : 1;
    } __PACKED;
    uint8_t seg_misc[3]; /* need to change to big endian */
} meshx_lower_trans_seg_access_misc_t;

typedef struct
{
    meshx_lower_trans_access_pdu_metadata_t metadata;
    meshx_lower_trans_seg_access_misc_t seg_misc;
    uint8_t pdu[MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE];
} __PACKED meshx_lower_trans_seg_access_pdu_t;

/* control message NetMIC is fixed to 64bit, no TransMIC */
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
        } __PACKED;
        uint8_t seg_misc[3]; /* need to chang to big endian */
    };
    uint8_t pdu[MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE];
} __PACKED meshx_lower_trans_seg_ctl_pdu_t;


static void meshx_lower_transport_tx_task_run(meshx_lower_trans_tx_task_t *ptx_task);

int32_t meshx_lower_transport_init(void)
{
    meshx_lower_trans_tx_task_t *ptx_tasks = meshx_malloc(meshx_node_params.config.trans_tx_task_num *
                                                          sizeof(
                                                              meshx_lower_trans_tx_task_t));
    if (NULL == ptx_tasks)
    {
        MESHX_ERROR("initialize lower transport failed: tx out of memory!");
        return -MESHX_ERR_MEM;
    }

    meshx_lower_trans_rx_task_t *prx_tasks = meshx_malloc(meshx_node_params.config.trans_rx_task_num *
                                                          sizeof(
                                                              meshx_lower_trans_rx_task_t));
    if (NULL == prx_tasks)
    {
        meshx_free(ptx_tasks);
        MESHX_ERROR("initialize lower transport failed: rx out of memory!");
        return -MESHX_ERR_MEM;
    }

    meshx_list_init_head(&meshx_lower_trans_tx_task_idle);
    meshx_list_init_head(&meshx_lower_trans_tx_task_active);
    meshx_list_init_head(&meshx_lower_trans_tx_task_pending);
    memset(ptx_tasks, 0, meshx_node_params.config.trans_tx_task_num * sizeof(
               meshx_lower_trans_tx_task_t));
    for (uint8_t i = 0; i < meshx_node_params.config.trans_tx_task_num ; ++i)
    {
        meshx_list_append(&meshx_lower_trans_tx_task_idle, &ptx_tasks[i].node);
    }

    meshx_list_init_head(&meshx_lower_trans_rx_task_idle);
    meshx_list_init_head(&meshx_lower_trans_rx_task_active);
    memset(prx_tasks, 0, meshx_node_params.config.trans_rx_task_num * sizeof(
               meshx_lower_trans_rx_task_t));
    for (uint8_t i = 0; i < meshx_node_params.config.trans_rx_task_num ; ++i)
    {
        meshx_list_append(&meshx_lower_trans_rx_task_idle, &prx_tasks[i].node);
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

#if 0
static void meshx_lower_trans_rx_task_release(meshx_lower_trans_rx_task_t *ptask)
{
    MESHX_ASSERT(NULL != ptask);
    MESHX_DEBUG("release task: 0x%08x", ptask);
    if (NULL != ptask->ack_timer)
    {
        meshx_timer_delete(ptask->ack_timer);
        ptask->ack_timer = NULL;
    }
    if (NULL != ptask->ppdu)
    {
        meshx_free(ptask->ppdu);
        ptask->ppdu = NULL;
    }
    meshx_list_append(&meshx_lower_trans_rx_task_idle, &ptask->node);
}
#endif

static void meshx_lower_trans_timeout_handler(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

static void meshx_lower_trans_send_seg_msg(meshx_network_if_t network_if, const uint8_t *ppdu,
                                           uint16_t pdu_len, const meshx_msg_tx_ctx_t *pmsg_tx_ctx)
{
    MESHX_INFO("send seg message");
    uint8_t seg_num = (pdu_len + MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE - 1) /
                      MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
    /* send segment message for the first time */
    uint8_t seg_len;
    uint16_t data_offset = 0;
    meshx_lower_trans_seg_access_pdu_t pdu;
    pdu.metadata.aid = pmsg_tx_ctx->aid;
    pdu.metadata.akf = pmsg_tx_ctx->akf;
    pdu.metadata.seg = 1;
    pdu.seg_misc.szmic = pmsg_tx_ctx->szmic;
    for (uint8_t i = 0; i < seg_num; ++i)
    {
        pdu.seg_misc.seq_zero = pmsg_tx_ctx->seq_zero;
        pdu.seg_misc.sego = i;
        pdu.seg_misc.segn = seg_num - 1;

        /* TODO: use other way to change endianess */
        uint8_t temp = pdu.seg_misc.seg_misc[0];
        pdu.seg_misc.seg_misc[0] = pdu.seg_misc.seg_misc[2];
        pdu.seg_misc.seg_misc[2] = temp;

        seg_len = (seg_num == (i + 1)) ? (pdu_len - data_offset) :
                  MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
        memcpy(pdu.pdu, ppdu + data_offset, seg_len);
        data_offset += seg_len;
        seg_len += (sizeof(meshx_lower_trans_access_pdu_metadata_t) + 3);
        MESHX_DEBUG("send seg pdu: %d", i);
        MESHX_DUMP_DEBUG(&pdu, seg_len);
        meshx_network_send(network_if, (const uint8_t *)&pdu, seg_len, pmsg_tx_ctx);
    }
}

static void meshx_lower_trans_tx_task_release(meshx_lower_trans_tx_task_t *ptask)
{
    MESHX_ASSERT(NULL != ptask);
    meshx_list_remove(&ptask->node);
    MESHX_DEBUG("release task: 0x%08x", ptask);
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
    meshx_list_append(&meshx_lower_trans_tx_task_idle, &ptask->node);
}

static void meshx_lower_transport_tx_task_finish(meshx_lower_trans_tx_task_t *ptx_task)
{
    meshx_lower_trans_tx_task_release(ptx_task);

    meshx_list_t *ppending_node;
    meshx_lower_trans_tx_task_t *ppending_task = NULL;
    meshx_list_t *pactive_node;
    meshx_lower_trans_tx_task_t *pactive_task = NULL;
    bool task_execute = FALSE;
    meshx_list_foreach(ppending_node, &meshx_lower_trans_tx_task_pending)
    {
        task_execute = TRUE;
        ppending_task = MESHX_CONTAINER_OF(ppending_node, meshx_lower_trans_tx_task_t, node);
        meshx_list_foreach(pactive_node, &meshx_lower_trans_tx_task_active)
        {
            pactive_task = MESHX_CONTAINER_OF(pactive_node, meshx_lower_trans_tx_task_t, node);
            if (ppending_task->msg_tx_ctx.dst == pactive_task->msg_tx_ctx.dst)
            {
                task_execute = FALSE;
            }
        }
        if (task_execute)
        {
            MESHX_INFO("remove lower trans task from pending: 0x%08x", ppending_task);
            break;
        }
    }

    if (task_execute)
    {
        meshx_list_remove(&ppending_task->node);
        meshx_lower_transport_tx_task_run(ppending_task);
    }
}

static void meshx_lower_trans_handle_tx_timeout(meshx_lower_trans_tx_task_t *ptask)
{
    ptask->retry_times ++;
    if (ptask->retry_times > MESHX_LOWER_TRANS_MAX_RETRY_TIMES)
    {
        if (MESHX_ADDRESS_IS_UNICAST(ptask->msg_tx_ctx.dst))
        {
            /* TODO: notify upper transport send failed, no ack from remote or some segment can't be received */
        }
        else
        {
            /* TODO: notify upper transport send finished */
        }

        meshx_lower_transport_tx_task_finish(ptask);
    }
    else
    {
        /* send segment message */
        meshx_lower_trans_send_seg_msg(ptask->network_if, ptask->ppdu, ptask->pdu_len, &ptask->msg_tx_ctx);
    }
}

void meshx_lower_trans_async_handle_timeout(meshx_async_msg_t msg)
{
    meshx_lower_trans_handle_tx_timeout(msg.pdata);
}

static void meshx_lower_transport_tx_task_run(meshx_lower_trans_tx_task_t *ptx_task)
{
    MESHX_ASSERT(NULL != ptx_task);

    if (MESHX_SUCCESS != meshx_timer_create(&ptx_task->retry_timer, MESHX_TIMER_MODE_REPEATED,
                                            meshx_lower_trans_timeout_handler, ptx_task))
    {
        MESHX_ERROR("send lower transport segment failed: out of resource!");
        meshx_lower_transport_tx_task_finish(ptx_task);
        return ;
    }

    MESHX_INFO("run lower trans task: 0x%08x", ptx_task);
    meshx_list_append(&meshx_lower_trans_tx_task_active, &ptx_task->node);
    /* check destination address */
    if (MESHX_ADDRESS_IS_UNICAST(ptx_task->msg_tx_ctx.dst))
    {
        /* set timer to retrans delay */
        meshx_timer_start(ptx_task->retry_timer,
                          MESHX_LOWER_TRANS_TX_RETRY_BASE + MESHX_LOWER_TRANS_TX_RETRY_TTL_FACTOR * ptx_task->msg_tx_ctx.ttl);
    }
    else
    {
        /* set timer to small random delay */
        meshx_timer_start(ptx_task->retry_timer, meshx_lower_trans_random());
    }

    /* send segment message for the first time */
    meshx_lower_trans_send_seg_msg(ptx_task->network_if, ptx_task->ppdu, ptx_task->pdu_len,
                                   &ptx_task->msg_tx_ctx);
}

static void meshx_lower_transport_tx_task_try(meshx_lower_trans_tx_task_t *ptx_task)
{
    MESHX_ASSERT(NULL != ptx_task);

    meshx_list_t *pnode;
    meshx_lower_trans_tx_task_t *pcur_task;
    meshx_list_foreach(pnode, &meshx_lower_trans_tx_task_active)
    {
        pcur_task = MESHX_CONTAINER_OF(pnode, meshx_lower_trans_tx_task_t, node);
        if (pcur_task->msg_tx_ctx.dst == ptx_task->msg_tx_ctx.dst)
        {
            /* can't send segment message to same dst at a time */
            MESHX_INFO("pending lower trans task: 0x%08x", ptx_task);
            meshx_list_append(&meshx_lower_trans_tx_task_pending, pnode);
            return ;
        }
    }

    /* can send task now */
    meshx_lower_transport_tx_task_run(ptx_task);
}

static meshx_lower_trans_tx_task_t *meshx_lower_trans_tx_task_request(uint16_t pdu_len)
{
    if (meshx_list_is_empty(&meshx_lower_trans_tx_task_idle))
    {
        return NULL;
    }

    meshx_list_t *pnode = meshx_list_pop(&meshx_lower_trans_tx_task_idle);
    MESHX_ASSERT(NULL != pnode);
    meshx_lower_trans_tx_task_t *ptask =  MESHX_CONTAINER_OF(pnode, meshx_lower_trans_tx_task_t, node);
    ptask->ppdu = meshx_malloc(pdu_len);
    if (NULL == ptask->ppdu)
    {
        meshx_list_append(pnode, &meshx_lower_trans_tx_task_idle);
        MESHX_ERROR("request lower transport task failed: out of memory!");
        return NULL;
    }

    ptask->retry_times = 0;

    return ptask;
}

static __INLINE uint64_t meshx_lower_trans_seq_auth(uint32_t iv_index, uint32_t seq)
{
    uint64_t seq_auth = iv_index;
    seq_auth <<= 24;
    seq_auth += seq;

    return seq_auth;
}

#if 0
static bool meshx_lower_trans_is_sending(uint16_t dst)
{
    return TRUE;
}
#endif

int32_t meshx_lower_transport_send(meshx_network_if_t network_if, const uint8_t *pupper_trans_pdu,
                                   uint16_t pdu_len, meshx_msg_tx_ctx_t *pmsg_tx_ctx)
{
    if (0 == pdu_len)
    {
        MESHX_ERROR("no valid message!");
        return -MESHX_ERR_LENGTH;
    }

    if (!MESHX_ADDRESS_IS_VALID(pmsg_tx_ctx->dst))
    {
        MESHX_ERROR("invlaid destination address: 0x%4x", pmsg_tx_ctx->dst);
        return -MESHX_ERR_INVAL;
    }

    if (pmsg_tx_ctx->ctl)
    {
        /* control message */
        if (pmsg_tx_ctx->force_seg)
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
        if (pmsg_tx_ctx->force_seg || (pdu_len > MESHX_LOWER_TRANS_UNSEG_ACCESS_MAX_PDU_SIZE))
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
            meshx_lower_trans_tx_task_t *ptask = meshx_lower_trans_tx_task_request(pdu_len);
            if (NULL == ptask)
            {
                MESHX_ERROR("lower transport is busy now, try again later!");
                return -MESHX_ERR_BUSY;
            }
            ptask->network_if = network_if;
            ptask->msg_tx_ctx = *pmsg_tx_ctx;
            memcpy(ptask->ppdu, pupper_trans_pdu, pdu_len);
            ptask->pdu_len = pdu_len;

            meshx_lower_transport_tx_task_try(ptask);
        }
        else
        {
            /* unsegment message */
            meshx_lower_trans_unseg_access_pdu_t pdu;
            pdu.metadata.aid = pmsg_tx_ctx->aid;
            pdu.metadata.akf = pmsg_tx_ctx->akf;
            pdu.metadata.seg = 0;
            memcpy(pdu.pdu, pupper_trans_pdu, pdu_len);
            meshx_network_send(network_if, (const uint8_t *)&pdu,
                               sizeof(meshx_lower_trans_access_pdu_metadata_t) + pdu_len, pmsg_tx_ctx);
        }
    }

    return MESHX_SUCCESS;
}

static meshx_lower_trans_rx_task_t *meshx_lower_trans_rx_task_request(uint16_t max_pdu_len,
                                                                      meshx_msg_rx_ctx_t *prx_ctx)
{
    MESHX_ASSERT(NULL != prx_ctx);
    /* check exists */
    meshx_list_t *pnode;
    meshx_lower_trans_rx_task_t *prx_task;
    meshx_list_foreach(pnode, &meshx_lower_trans_rx_task_active)
    {
        prx_task = MESHX_CONTAINER_OF(pnode, meshx_lower_trans_rx_task_t, node);
        if (prx_task->msg_rx_ctx.src == prx_ctx->src)
        {
            uint64_t seq_auth_rx = meshx_lower_trans_seq_auth(prx_ctx->iv_index, prx_ctx->seq_origin);
            uint64_t seq_auth_store = meshx_lower_trans_seq_auth(prx_task->msg_rx_ctx.iv_index,
                                                                 prx_task->msg_rx_ctx.seq_origin);
            if (seq_auth_rx > seq_auth_store)
            {
                if (NULL != prx_task->ppdu)
                {
                    meshx_free(prx_task->ppdu);
                    prx_task->ppdu = NULL;
                }
                if (0 != prx_task->not_received_seg)
                {
                    /* receive new seg message, cancel old message */
                    MESHX_WARN("receive new seg message, cancel old: seq origin 0x%08x",
                               prx_task->msg_rx_ctx.seq_origin);
                }
                meshx_list_remove(&prx_task->node);
                meshx_list_append(&meshx_lower_trans_rx_task_idle, &prx_task->node);
                goto NEW_SEG_MSG;
            }
            else if (seq_auth_rx == seq_auth_store)
            {
                /* receive new segment or dumplicate segment */
                return prx_task;
            }
            else
            {
                /* receive old seg message, abandom */
                MESHX_INFO("receive old segment: iv index 0x%08x, seq origin 0x%08x", prx_ctx->iv_index,
                           prx_ctx->seq_origin);
                return NULL;
            }
        }
    }

NEW_SEG_MSG:
    /* first receive */
    if (meshx_list_is_empty(&meshx_lower_trans_rx_task_idle))
    {
        MESHX_ERROR("request rx task failed: busy!");
        return NULL;
    }

    pnode = meshx_list_pop(&meshx_lower_trans_rx_task_idle);
    meshx_lower_trans_rx_task_t *ptask =  MESHX_CONTAINER_OF(pnode, meshx_lower_trans_rx_task_t, node);
    if (MESHX_SUCCESS != meshx_timer_create(&ptask->ack_timer, MESHX_TIMER_MODE_SINGLE_SHOT,
                                            meshx_lower_trans_timeout_handler, ptask))
    {
        meshx_list_append(pnode, &meshx_lower_trans_rx_task_idle);
        MESHX_ERROR("request rx task failed: create ack timer failed!");
        return NULL;
    }
    if (MESHX_SUCCESS != meshx_timer_create(&ptask->incomplete_timer, MESHX_TIMER_MODE_SINGLE_SHOT,
                                            meshx_lower_trans_timeout_handler, ptask))
    {
        meshx_timer_delete(ptask->ack_timer);
        meshx_list_append(pnode, &meshx_lower_trans_rx_task_idle);
        MESHX_ERROR("request rx task failed: create incomplete timer failed!");
        return NULL;
    }

    ptask->ppdu = meshx_malloc(max_pdu_len);
    if (NULL == ptask->ppdu)
    {
        meshx_timer_delete(ptask->ack_timer);
        meshx_timer_delete(ptask->incomplete_timer);
        meshx_list_append(pnode, &meshx_lower_trans_rx_task_idle);
        MESHX_ERROR("request rx task failed: out of memory!");
        return NULL;
    }
    ptask->max_pdu_len = max_pdu_len;

    ptask->msg_rx_ctx = *prx_ctx;
    /* initialize segment flag */
    uint8_t seg_num = 0;
    if (prx_ctx->ctl)
    {
        seg_num = max_pdu_len / MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE;
    }
    else
    {
        seg_num = max_pdu_len / MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
    }

    for (uint8_t i = 0; i < seg_num; ++i)
    {
        ptask->not_received_seg |= (1 << i);
    }

    meshx_list_append(pnode, &meshx_lower_trans_rx_task_active);

    return ptask;
}

int32_t meshx_lower_transport_receive(meshx_network_if_t network_if, const uint8_t *pdata,
                                      uint8_t len, meshx_msg_rx_ctx_t *pmsg_rx_ctx)
{
    MESHX_DEBUG("receive lower transport data:");
    MESHX_DUMP_DEBUG(pdata, len);
    if (pmsg_rx_ctx->ctl)
    {
        /* control message */
    }
    else
    {
        /* access message */
        const meshx_lower_trans_access_pdu_metadata_t *pmetadata = (const
                                                                    meshx_lower_trans_access_pdu_metadata_t *)pdata;
        if (pmetadata->seg)
        {
            /* segmented access message */
            const meshx_lower_trans_seg_access_pdu_t *pseg_access_msg = (const
                                                                         meshx_lower_trans_seg_access_pdu_t *)pdata;
            meshx_lower_trans_seg_access_misc_t seg_misc = pseg_access_msg->seg_misc;
            /* TODO: use other way to change endianess */
            uint8_t temp = seg_misc.seg_misc[0];
            seg_misc.seg_misc[0] = seg_misc.seg_misc[2];
            seg_misc.seg_misc[2] = temp;

            /* store segment message */
            uint16_t max_pdu_len = (seg_misc.segn + 1) * MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
            meshx_lower_trans_rx_task_t *ptask = meshx_lower_trans_rx_task_request(max_pdu_len, pmsg_rx_ctx);

            if (NULL == ptask)
            {
                /* TODO: ack can't receive */
                return -MESHX_ERR_BUSY;
            }

            if (ptask->max_pdu_len != max_pdu_len)
            {
                MESHX_ERROR("receive segment length dismatch with previous: %d-%d", max_pdu_len,
                            ptask->max_pdu_len);
                /* TODO: ack can't receive */
                return -MESHX_ERR_LENGTH;
            }

            if (0 == ptask->not_received_seg)
            {
                MESHX_INFO("already received all segments");
                /* TODO: ack immedietely */
                return MESHX_SUCCESS;
            }

            if (0 == seg_misc.segn)
            {
                /* only one segment */
                ptask->not_received_seg = 0;
                /* TODO: ack immedietely */
            }
            else
            {
                /* multiple segment */
                /* store segment */
                uint8_t seg_len = (seg_misc.sego == seg_misc.segn) ? len - sizeof(
                                      meshx_lower_trans_access_pdu_metadata_t) - sizeof(meshx_lower_trans_seg_access_misc_t) :
                                  MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
                memcpy(ptask->ppdu + seg_misc.sego * MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE,
                       pseg_access_msg->pdu, seg_len);
                ptask->not_received_seg &= ~(1 << seg_misc.sego);
                if (0 == ptask->not_received_seg)
                {
                    /* receive all segments */
                    /* TODO: ack immediately and stop ack timer */
                }
                else
                {
                    /* start ack timer and restart incomplete timer */
                    if (!meshx_timer_is_active(ptask->ack_timer))
                    {
                        meshx_timer_start(ptask->ack_timer,
                                          MESHX_LOWER_TRANS_RX_ACK_BASE + MESHX_LOWER_TRANS_RX_ACK_TTL_FACTOR * pmsg_rx_ctx->ttl);
                    }
                    meshx_timer_restart(ptask->incomplete_timer, MESHX_LOWER_TRANS_INCOMPLETE_TIMEOUT);
                }
            }
        }
        else
        {
            /* unsegment access message */
        }


    }

    return MESHX_SUCCESS;
}

