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
#include "meshx_lib.h"
#include "meshx_endianness.h"

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

#define MESHX_LOWER_TRANS_TX_RETRY_BASE                         200    /* TODO: can configure */
#define MESHX_LOWER_TRANS_TX_RETRY_TTL_FACTOR                   50 /* 50 * ttl */ /* TODO: can configure */

#define MESHX_LOWER_TRANS_MAX_SEQ_DELTA                         0x2000

#define MESHX_LOWER_TRANS_INCOMPLETE_TIMEOUT                    10000 /* ms */
#define MESHX_LOWER_TRANS_RX_ACK_BASE                           150 /* TODO: can configure */
#define MESHX_LOWER_TRANS_RX_ACK_TTL_FACTOR                     50 /* 50 * ttl */ /* TODO: can configure */

#define MESHX_LOWER_TRANS_STORE_TIMEOUT                         10000 /* ms */

/* lower transport tx task */
typedef struct
{
    meshx_network_if_t network_if;
    meshx_msg_tx_ctx_t msg_tx_ctx;
    uint8_t *ppdu;
    uint16_t pdu_len;
    uint32_t seg_bits;
    uint32_t block_ack;
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
    uint32_t block_ack;
    meshx_timer_t ack_timer;
    meshx_timer_t incomplete_timer;
    meshx_list_t node;
} meshx_lower_trans_rx_task_t;

/* rx active task shall not exist same dst */
static meshx_list_t meshx_lower_trans_rx_task_idle;
static meshx_list_t meshx_lower_trans_rx_task_active;



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
} __PACKED meshx_lower_trans_ctl_pdu_metadata_t;

typedef struct
{
    meshx_lower_trans_ctl_pdu_metadata_t metadata;
    uint8_t pdu[MESHX_LOWER_TRANS_UNSEG_CTL_MAX_PDU_SIZE];
} __PACKED meshx_lower_trans_unseg_ctl_pdu_t;

typedef union
{
    struct
    {
        uint32_t segn : 5;
        uint32_t sego : 5;
        uint32_t seq_zero : 13;
        uint32_t rfu : 1;
    } __PACKED;
    uint8_t seg_misc[3]; /* need to change to big endian */
} meshx_lower_trans_seg_ctl_misc_t;

typedef struct
{
    meshx_lower_trans_ctl_pdu_metadata_t metadata;
    meshx_lower_trans_seg_ctl_misc_t seg_misc;
    uint8_t pdu[MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE];
} __PACKED meshx_lower_trans_seg_ctl_pdu_t;

typedef struct
{
    union
    {
        struct
        {
            uint16_t rfu : 2;
            uint16_t seq_zero : 13;
            uint16_t obo : 1;
        } __PACKED;
        uint16_t ack_misc;
    };
    uint32_t block_ack;
} __PACKED meshx_lower_trans_seg_ack_pdu_t;



static int32_t meshx_lower_trans_tx_task_run(meshx_lower_trans_tx_task_t *ptx_task);

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

static void meshx_lower_trans_tx_timeout_handler(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_TX;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

static int32_t meshx_lower_trans_send_seg_msg(meshx_network_if_t network_if,
                                              const uint8_t *ppdu,
                                              uint16_t pdu_len, uint32_t block_ack, const meshx_msg_tx_ctx_t *pmsg_tx_ctx)
{
    int32_t ret = MESHX_SUCCESS;
    uint8_t seg_num;
    if (pmsg_tx_ctx->ctl)
    {
        seg_num = (pdu_len + MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE - 1) /
                  MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE;
    }
    else
    {
        seg_num = (pdu_len + MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE - 1) /
                  MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
    }


    uint8_t seg_len;
    uint16_t data_offset = 0;
    uint8_t pdu[20];

    if (pmsg_tx_ctx->ctl)
    {
        meshx_lower_trans_seg_ctl_pdu_t *pctl_pdu = (meshx_lower_trans_seg_ctl_pdu_t *)pdu;
        pctl_pdu->metadata.opcode = pmsg_tx_ctx->opcode;
        pctl_pdu->metadata.seg = 1;
    }
    else
    {
        meshx_lower_trans_seg_access_pdu_t *paccess_pdu = (meshx_lower_trans_seg_access_pdu_t *)pdu;
        if (pmsg_tx_ctx->akf)
        {
            paccess_pdu->metadata.aid = pmsg_tx_ctx->papp_key->aid;
        }
        else
        {
            paccess_pdu->metadata.aid = 0;
        }
        paccess_pdu->metadata.akf = pmsg_tx_ctx->akf;
        paccess_pdu->metadata.seg = 1;
    }
    for (uint8_t i = 0; i < seg_num; ++i)
    {
        if (block_ack & (1 << i))
        {
            continue;
        }

        if (pmsg_tx_ctx->ctl)
        {
            meshx_lower_trans_seg_ctl_pdu_t *pctl_pdu = (meshx_lower_trans_seg_ctl_pdu_t *)pdu;

            seg_len = (seg_num == (i + 1)) ? (pdu_len - data_offset) :
                      MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE;

            pctl_pdu->seg_misc.seq_zero = pmsg_tx_ctx->seq_zero;
            pctl_pdu->seg_misc.sego = i;
            pctl_pdu->seg_misc.segn = seg_num - 1;
            meshx_swap(pctl_pdu->seg_misc.seg_misc, pctl_pdu->seg_misc.seg_misc + 2);
            memcpy(pctl_pdu->pdu, ppdu + data_offset, seg_len);
        }
        else
        {
            meshx_lower_trans_seg_access_pdu_t *paccess_pdu = (meshx_lower_trans_seg_access_pdu_t *)pdu;

            seg_len = (seg_num == (i + 1)) ? (pdu_len - data_offset) :
                      MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;

            paccess_pdu->seg_misc.szmic = pmsg_tx_ctx->szmic;
            paccess_pdu->seg_misc.seq_zero = pmsg_tx_ctx->seq_zero;
            paccess_pdu->seg_misc.sego = i;
            paccess_pdu->seg_misc.segn = seg_num - 1;
            meshx_swap(paccess_pdu->seg_misc.seg_misc, paccess_pdu->seg_misc.seg_misc + 2);
            memcpy(paccess_pdu->pdu, ppdu + data_offset, seg_len);
        }

        data_offset += seg_len;
        if (pmsg_tx_ctx->ctl)
        {
            seg_len += (sizeof(meshx_lower_trans_ctl_pdu_metadata_t) + sizeof(
                            meshx_lower_trans_seg_ctl_misc_t));
        }
        else
        {
            seg_len += (sizeof(meshx_lower_trans_access_pdu_metadata_t) + sizeof(
                            meshx_lower_trans_seg_access_misc_t));
        }
        MESHX_DEBUG("send access seg pdu: %d", i);
        MESHX_DUMP_DEBUG(pdu, seg_len);
        ret = meshx_network_send(network_if, pdu, seg_len, pmsg_tx_ctx);
        if (MESHX_SUCCESS != ret)
        {
            break;
        }
    }

    return ret;
}

static void meshx_lower_trans_tx_task_release(meshx_lower_trans_tx_task_t *ptask)
{
    MESHX_ASSERT(NULL != ptask);
    meshx_list_remove(&ptask->node);
    MESHX_DEBUG("release task(0x%08x)", ptask);
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

static void meshx_lower_trans_tx_task_finish(meshx_lower_trans_tx_task_t *ptx_task)
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
            MESHX_INFO("remove lower trans task(0x%08x) from pending", ppending_task);
            break;
        }
    }

    if (task_execute)
    {
        meshx_list_remove(&ppending_task->node);
        meshx_lower_trans_tx_task_run(ppending_task);
    }
}

static void meshx_lower_trans_handle_tx_timeout(meshx_lower_trans_tx_task_t *ptask)
{
    ptask->retry_times ++;
    if (ptask->retry_times > meshx_node_params.config.trans_tx_retry_times)
    {
        if (MESHX_ADDRESS_IS_UNICAST(ptask->msg_tx_ctx.dst))
        {
            /* TODO: notify upper transport send failed, no ack from remote or some segment can't be received */
        }
        else
        {
            /* TODO: notify upper transport send finished */
        }

        meshx_lower_trans_tx_task_finish(ptask);
    }
    else
    {
        /* send segment message */
        meshx_lower_trans_send_seg_msg(ptask->network_if, ptask->ppdu, ptask->pdu_len,
                                       ptask->block_ack,
                                       &ptask->msg_tx_ctx);
    }
}

void meshx_lower_trans_async_handle_tx_timeout(meshx_async_msg_t msg)
{
    meshx_lower_trans_handle_tx_timeout(msg.pdata);
}

static void meshx_lower_trans_tx_timer_start(const meshx_lower_trans_tx_task_t *ptx_task)
{
    /* check destination address */
    if (MESHX_ADDRESS_IS_UNICAST(ptx_task->msg_tx_ctx.dst))
    {
        /* set timer to retrans delay */
        if (meshx_timer_is_active(ptx_task->retry_timer))
        {
            meshx_timer_restart(ptx_task->retry_timer,
                                MESHX_LOWER_TRANS_TX_RETRY_BASE + MESHX_LOWER_TRANS_TX_RETRY_TTL_FACTOR * ptx_task->msg_tx_ctx.ttl);
        }
        else
        {
            meshx_timer_start(ptx_task->retry_timer,
                              MESHX_LOWER_TRANS_TX_RETRY_BASE + MESHX_LOWER_TRANS_TX_RETRY_TTL_FACTOR * ptx_task->msg_tx_ctx.ttl);
        }
    }
    else
    {
        /* set timer to small random delay */
        meshx_timer_start(ptx_task->retry_timer, meshx_lower_trans_random());
    }
}

static int32_t meshx_lower_trans_tx_task_run(meshx_lower_trans_tx_task_t *ptx_task)
{
    MESHX_ASSERT(NULL != ptx_task);

    if (MESHX_SUCCESS != meshx_timer_create(&ptx_task->retry_timer, MESHX_TIMER_MODE_SINGLE_SHOT,
                                            meshx_lower_trans_tx_timeout_handler, ptx_task))
    {
        MESHX_ERROR("send lower transport segment failed: out of resource!");
        meshx_lower_trans_tx_task_finish(ptx_task);
        return -MESHX_ERR_RESOURCE;
    }

    MESHX_INFO("run lower trans task(0x%08x)", ptx_task);
    meshx_list_append(&meshx_lower_trans_tx_task_active, &ptx_task->node);

    /* start retrans timer */
    meshx_lower_trans_tx_timer_start(ptx_task);

    /* send segment message for the first time */
    return meshx_lower_trans_send_seg_msg(ptx_task->network_if, ptx_task->ppdu, ptx_task->pdu_len,
                                          ptx_task->block_ack,
                                          &ptx_task->msg_tx_ctx);
}

static int32_t meshx_lower_transport_tx_task_try(meshx_lower_trans_tx_task_t *ptx_task)
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
            return MESHX_SUCCESS;
        }
    }

    /* can send task now */
    return meshx_lower_trans_tx_task_run(ptx_task);
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
        meshx_list_append(&meshx_lower_trans_tx_task_idle, pnode);
        MESHX_ERROR("request lower transport task failed: out of memory!");
        return NULL;
    }

    ptask->retry_times = 0;
    ptask->block_ack = 0;
    ptask->seg_bits = 0;

    return ptask;
}

static __INLINE uint64_t meshx_lower_trans_seq_auth(uint32_t iv_index, uint32_t seq)
{
    uint64_t seq_auth = iv_index;
    seq_auth <<= 24;
    seq_auth += seq;

    return seq_auth;
}

static bool meshx_lower_trans_is_tx_task_exist(const meshx_msg_tx_ctx_t *pmsg_tx_ctx)
{
    meshx_list_t *pnode;
    meshx_lower_trans_tx_task_t *pcur_task;
    meshx_list_foreach(pnode, &meshx_lower_trans_tx_task_active)
    {
        pcur_task = MESHX_CONTAINER_OF(pnode, meshx_lower_trans_tx_task_t, node);
        if ((pcur_task->msg_tx_ctx.dst == pmsg_tx_ctx->dst) &&
            (pcur_task->msg_tx_ctx.seq_zero == pmsg_tx_ctx->seq_zero))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static int32_t meshx_lower_trans_process_seg_msg(meshx_network_if_t network_if,
                                                 const uint8_t *pupper_trans_pdu,
                                                 uint16_t pdu_len, uint8_t max_seg_size, const meshx_msg_tx_ctx_t *pmsg_tx_ctx)
{
    /* segment message */
    uint8_t seg_num = (pdu_len + max_seg_size - 1) / max_seg_size;
    if (seg_num > MESHX_LOWER_TRANS_MAX_SEG_SIZE)
    {
        MESHX_ERROR("message length exceed maximum size: %d-%d", pdu_len,
                    max_seg_size * MESHX_LOWER_TRANS_MAX_SEG_SIZE);
        return -MESHX_ERR_LENGTH;
    }

    /* check exists first */
    if (meshx_lower_trans_is_tx_task_exist(pmsg_tx_ctx))
    {
        MESHX_ERROR("message already sending: dst 0x%04x, seq zero 0x%08x", pmsg_tx_ctx->dst,
                    pmsg_tx_ctx->seq_zero);
        return -MESHX_ERR_ALREADY;
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
    for (uint8_t i = 0; i < seg_num; ++i)
    {
        ptask->seg_bits |= (1 << i);
    }

    int32_t ret = meshx_lower_transport_tx_task_try(ptask);
    if (MESHX_SUCCESS != ret)
    {
        /* release task */
        meshx_lower_trans_tx_task_release(ptask);
    }

    return ret;
}

int32_t meshx_lower_transport_send(meshx_network_if_t network_if, const uint8_t *pupper_trans_pdu,
                                   uint16_t pdu_len, const meshx_msg_tx_ctx_t *pmsg_tx_ctx)
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

    int32_t ret = MESHX_SUCCESS;
    if (pmsg_tx_ctx->ctl)
    {
        /* control message */
        if (pmsg_tx_ctx->force_seg || (pdu_len > MESHX_LOWER_TRANS_UNSEG_CTL_MAX_PDU_SIZE))
        {
            /* segment message */
            ret = meshx_lower_trans_process_seg_msg(network_if, pupper_trans_pdu, pdu_len,
                                                    MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE, pmsg_tx_ctx);
        }
        else
        {
            meshx_lower_trans_unseg_ctl_pdu_t pdu;
            pdu.metadata.opcode = pmsg_tx_ctx->opcode;
            pdu.metadata.seg = 0;
            memcpy(pdu.pdu, pupper_trans_pdu, pdu_len);
            ret = meshx_network_send(network_if, (const uint8_t *)&pdu,
                                     sizeof(meshx_lower_trans_ctl_pdu_metadata_t) + pdu_len, pmsg_tx_ctx);
        }
    }
    else
    {
        /* access message */
        if (pmsg_tx_ctx->force_seg || (pdu_len > MESHX_LOWER_TRANS_UNSEG_ACCESS_MAX_PDU_SIZE))
        {
            /* segment message */
            ret = meshx_lower_trans_process_seg_msg(network_if, pupper_trans_pdu, pdu_len,
                                                    MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE, pmsg_tx_ctx);
        }
        else
        {
            /* unsegment message */
            meshx_lower_trans_unseg_access_pdu_t pdu;
            if (pmsg_tx_ctx->akf)
            {
                pdu.metadata.aid = pmsg_tx_ctx->papp_key->aid;
            }
            else
            {
                pdu.metadata.aid = 0;
            }
            pdu.metadata.akf = pmsg_tx_ctx->akf;
            pdu.metadata.seg = 0;
            memcpy(pdu.pdu, pupper_trans_pdu, pdu_len);
            ret = meshx_network_send(network_if, (const uint8_t *)&pdu,
                                     sizeof(meshx_lower_trans_access_pdu_metadata_t) + pdu_len, pmsg_tx_ctx);
        }
    }

    return ret;
}

static int32_t meshx_lower_transport_seg_ack(meshx_network_if_t network_if, uint32_t block_ack,
                                             meshx_msg_rx_ctx_t *pmsg_rx_ctx)
{
    meshx_lower_trans_seg_ack_pdu_t seg_ack;
    seg_ack.rfu = 0;
    seg_ack.seq_zero = pmsg_rx_ctx->seq_origin;
    seg_ack.obo = 0;
    seg_ack.ack_misc = (seg_ack.ack_misc >> 8) | (seg_ack.ack_misc << 8);
    seg_ack.block_ack = MESHX_HOST_TO_BE32(block_ack);

    meshx_msg_tx_ctx_t msg_ctx;
    memset(&msg_ctx, 0, sizeof(msg_ctx));
    msg_ctx.src = pmsg_rx_ctx->dst;
    msg_ctx.dst = pmsg_rx_ctx->src;
    msg_ctx.force_seg = 0;
    msg_ctx.ttl = meshx_node_params.param.default_ttl;
    msg_ctx.ctl = 1;
    msg_ctx.opcode = 0;
    msg_ctx.pnet_key = pmsg_rx_ctx->pnet_key;

    MESHX_DEBUG("lower transport block ack: 0x%08x", block_ack);
    return meshx_lower_transport_send(network_if, (const uint8_t *)&seg_ack,
                                      sizeof(seg_ack), &msg_ctx);
}

static void meshx_lower_trans_rx_task_release(meshx_lower_trans_rx_task_t *ptask)
{
    MESHX_ASSERT(NULL != ptask);
    meshx_list_remove(&ptask->node);
    MESHX_DEBUG("release rx task: 0x%08x", ptask);

    if (NULL != ptask->ack_timer)
    {
        meshx_timer_delete(ptask->ack_timer);
        ptask->ack_timer = NULL;
    }

    if (NULL != ptask->incomplete_timer)
    {
        meshx_timer_delete(ptask->incomplete_timer);
        ptask->incomplete_timer = NULL;
    }

    if (NULL != ptask->ppdu)
    {
        meshx_free(ptask->ppdu);
        ptask->ppdu = NULL;
    }
    meshx_list_append(&meshx_lower_trans_rx_task_idle, &ptask->node);
}

static void meshx_lower_trans_rx_ack_timeout_handler(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_RX_ACK;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

static void meshx_lower_trans_rx_incomplete_timeout_handler(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_RX_INCOMPLETE;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

void meshx_lower_trans_async_handle_rx_ack_timeout(meshx_async_msg_t msg)
{
    meshx_lower_trans_rx_task_t *ptask = msg.pdata;
    meshx_lower_transport_seg_ack(ptask->network_if, ptask->block_ack, &ptask->msg_rx_ctx);
}

void meshx_lower_trans_async_handle_rx_incomplete_timeout(meshx_async_msg_t msg)
{
    meshx_lower_trans_rx_task_t *ptask = msg.pdata;
    if (0 == ptask->not_received_seg)
    {
        /* receive all segments, cancel state store */
        MESHX_INFO("state store timeout!");
    }
    else
    {
        MESHX_ERROR("receive all segments timeout: not received segments 0x%08x", ptask->not_received_seg);
    }
    meshx_lower_trans_rx_task_release(ptask);
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
    prx_task =  MESHX_CONTAINER_OF(pnode, meshx_lower_trans_rx_task_t, node);
    if (MESHX_SUCCESS != meshx_timer_create(&prx_task->ack_timer, MESHX_TIMER_MODE_SINGLE_SHOT,
                                            meshx_lower_trans_rx_ack_timeout_handler, prx_task))
    {
        meshx_list_append(&meshx_lower_trans_rx_task_idle, pnode);
        MESHX_ERROR("request rx task failed: create ack timer failed!");
        return NULL;
    }
    if (MESHX_SUCCESS != meshx_timer_create(&prx_task->incomplete_timer, MESHX_TIMER_MODE_SINGLE_SHOT,
                                            meshx_lower_trans_rx_incomplete_timeout_handler, prx_task))
    {
        meshx_timer_delete(prx_task->ack_timer);
        meshx_list_append(&meshx_lower_trans_rx_task_idle, pnode);
        MESHX_ERROR("request rx task failed: create incomplete timer failed!");
        return NULL;
    }

    prx_task->ppdu = meshx_malloc(max_pdu_len);
    if (NULL == prx_task->ppdu)
    {
        meshx_timer_delete(prx_task->ack_timer);
        meshx_timer_delete(prx_task->incomplete_timer);
        meshx_list_append(&meshx_lower_trans_rx_task_idle, pnode);
        MESHX_ERROR("request rx task failed: out of memory!");
        return NULL;
    }
    prx_task->max_pdu_len = max_pdu_len;

    prx_task->msg_rx_ctx = *prx_ctx;
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
        prx_task->not_received_seg |= (1 << i);
    }
    prx_task->block_ack = 0;

    meshx_list_append(&meshx_lower_trans_rx_task_active, pnode);

    return prx_task;
}

static void meshx_lower_trans_recv_block_ack(const meshx_lower_trans_seg_ack_pdu_t *pseg_ack)
{
    meshx_lower_trans_seg_ack_pdu_t seg_ack = *pseg_ack;
    seg_ack.block_ack = MESHX_BE32_TO_HOST(seg_ack.block_ack);
    seg_ack.ack_misc = (seg_ack.ack_misc >> 8) | (seg_ack.ack_misc << 8);
    MESHX_INFO("receive seg ack: seq zero 0x%04x, obo %d, ack 0x%08x", seg_ack.seq_zero, seg_ack.obo,
               seg_ack.block_ack);

    /* find send task */
    meshx_list_t *pnode;
    meshx_lower_trans_tx_task_t *pcur_task;
    meshx_list_foreach(pnode, &meshx_lower_trans_tx_task_active)
    {
        pcur_task = MESHX_CONTAINER_OF(pnode, meshx_lower_trans_tx_task_t, node);
        if (pcur_task->msg_tx_ctx.seq_zero == seg_ack.seq_zero)
        {
            break;
        }
    }

    if (pnode != &meshx_lower_trans_tx_task_active)
    {
        /* find task */
        if (pcur_task->seg_bits == seg_ack.block_ack)
        {
            /* remote received all segments */
            /* TODO: notify upper transport send success */
            MESHX_INFO("remote receive all segments, seq zero(0x%04x)", pcur_task->msg_tx_ctx.seq_zero);
            meshx_lower_trans_tx_task_finish(pcur_task);
        }
        else
        {
            if (0 == seg_ack.block_ack)
            {
                /* TODO: notify upper transport send canceled, remote can't receive */
                MESHX_WARN("send canceled, remote can't receive!");
                meshx_lower_trans_tx_task_finish(pcur_task);
            }
            else
            {
                pcur_task->retry_times ++;
                if (pcur_task->retry_times > meshx_node_params.config.trans_tx_retry_times)
                {
                    if (MESHX_ADDRESS_IS_UNICAST(pcur_task->msg_tx_ctx.dst))
                    {
                        /* TODO: notify upper transport send failed, some segment can't be received */
                        MESHX_ERROR("segment message send failed: seq zero(0x%04x)", pcur_task->msg_tx_ctx.seq_zero);
                    }

                    meshx_lower_trans_tx_task_finish(pcur_task);
                }
                else
                {
                    pcur_task->block_ack = seg_ack.block_ack;
                    meshx_lower_trans_send_seg_msg(pcur_task->network_if, pcur_task->ppdu, pcur_task->pdu_len,
                                                   pcur_task->block_ack,
                                                   &pcur_task->msg_tx_ctx);
                    /* restart retrans timer */
                    meshx_lower_trans_tx_timer_start(pcur_task);
                }
            }
        }
    }
    else
    {
        MESHX_WARN("receive invalid segment ack: seq zero 0x%04x", pcur_task->msg_tx_ctx.seq_zero);
    }
}

static int32_t meshx_lower_trans_receive_seg_msg(meshx_network_if_t network_if,
                                                 const uint8_t *pdata,
                                                 uint8_t len, meshx_msg_rx_ctx_t *pmsg_rx_ctx)
{
    /* segmented access message */
    uint16_t max_pdu_len;
    uint8_t sego, segn;
    if (pmsg_rx_ctx->ctl)
    {
        const meshx_lower_trans_seg_ctl_pdu_t *pseg_ctl_msg = (const meshx_lower_trans_seg_ctl_pdu_t *)
                                                              pdata;
        meshx_lower_trans_seg_ctl_misc_t seg_misc = pseg_ctl_msg->seg_misc;
        meshx_swap(seg_misc.seg_misc, seg_misc.seg_misc + 2);

        /* store segment message */
        pmsg_rx_ctx->seq_origin = seg_misc.seq_zero;
        sego = seg_misc.sego;
        segn = seg_misc.segn;
        max_pdu_len = (seg_misc.segn + 1) * MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE;
    }
    else
    {
        const meshx_lower_trans_seg_access_pdu_t *pseg_access_msg = (const
                                                                     meshx_lower_trans_seg_access_pdu_t *)pdata;
        meshx_lower_trans_seg_access_misc_t seg_misc = pseg_access_msg->seg_misc;
        meshx_swap(seg_misc.seg_misc, seg_misc.seg_misc + 2);

        /* store segment message */
        pmsg_rx_ctx->seq_origin = seg_misc.seq_zero;
        sego = seg_misc.sego;
        segn = seg_misc.segn;
        max_pdu_len = (seg_misc.segn + 1) * MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
    }

    meshx_lower_trans_rx_task_t *ptask = meshx_lower_trans_rx_task_request(max_pdu_len, pmsg_rx_ctx);

    if (NULL == ptask)
    {
        meshx_lower_transport_seg_ack(network_if, 0, pmsg_rx_ctx);
        return -MESHX_ERR_BUSY;
    }

    if (ptask->max_pdu_len != max_pdu_len)
    {
        MESHX_ERROR("receive segment length dismatch with previous: %d-%d", max_pdu_len,
                    ptask->max_pdu_len);
        /* ack can't receive */
        /* TODO: release rx task */
        meshx_lower_transport_seg_ack(network_if, 0, pmsg_rx_ctx);
        return -MESHX_ERR_LENGTH;
    }

    if (0 == ptask->not_received_seg)
    {
        MESHX_INFO("already received all segments");
        /* ack immedietely */
        meshx_lower_transport_seg_ack(network_if, ptask->block_ack, pmsg_rx_ctx);
        return -MESHX_ERR_ALREADY;
    }

    if (0 == (ptask->not_received_seg & (1 << sego)))
    {
        MESHX_INFO("seg %d already received");
        return -MESHX_ERR_ALREADY;
    }

    if (0 == segn)
    {
        /* only one segment */
        ptask->not_received_seg = 0;
        ptask->block_ack = 0x01;
        if (pmsg_rx_ctx->ctl)
        {
            const meshx_lower_trans_seg_ctl_pdu_t *pseg_ctl_msg = (const meshx_lower_trans_seg_ctl_pdu_t *)
                                                                  pdata;
            ptask->pdu_len = len - sizeof(meshx_lower_trans_ctl_pdu_metadata_t) - sizeof(
                                 meshx_lower_trans_seg_ctl_misc_t);
            memcpy(ptask->ppdu, pseg_ctl_msg->pdu, ptask->pdu_len);
        }
        else
        {
            const meshx_lower_trans_seg_access_pdu_t *pseg_access_msg = (const
                                                                         meshx_lower_trans_seg_access_pdu_t *)pdata;
            ptask->pdu_len = len - sizeof(meshx_lower_trans_access_pdu_metadata_t) - sizeof(
                                 meshx_lower_trans_seg_access_misc_t);
            memcpy(ptask->ppdu, pseg_access_msg->pdu, ptask->pdu_len);
        }
        meshx_lower_transport_seg_ack(network_if, ptask->block_ack, pmsg_rx_ctx);

        /* stop timer */
        meshx_timer_delete(ptask->ack_timer);
        ptask->ack_timer = NULL;
        meshx_timer_restart(ptask->incomplete_timer, MESHX_LOWER_TRANS_STORE_TIMEOUT);

        /* TODO: notify upper transport layer */
        MESHX_DEBUG("receive upper transport pdu: type %d", pmsg_rx_ctx->ctl);
        MESHX_DUMP_DEBUG(ptask->ppdu, ptask->pdu_len);
    }
    else
    {
        /* multiple segment */
        /* store segment */
        uint8_t seg_len;
        if (pmsg_rx_ctx->ctl)
        {
            const meshx_lower_trans_seg_ctl_pdu_t *pseg_ctl_msg = (const
                                                                   meshx_lower_trans_seg_ctl_pdu_t *)pdata;
            seg_len = (sego == segn) ? len - sizeof(
                          meshx_lower_trans_ctl_pdu_metadata_t) - sizeof(meshx_lower_trans_seg_ctl_misc_t) :
                      MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE;
            memcpy(ptask->ppdu + sego * MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE,
                   pseg_ctl_msg->pdu, seg_len);
        }
        else
        {
            const meshx_lower_trans_seg_access_pdu_t *pseg_access_msg = (const
                                                                         meshx_lower_trans_seg_access_pdu_t *)pdata;
            seg_len = (sego == segn) ? len - sizeof(
                          meshx_lower_trans_access_pdu_metadata_t) - sizeof(meshx_lower_trans_seg_access_misc_t) :
                      MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
            memcpy(ptask->ppdu + sego * MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE,
                   pseg_access_msg->pdu, seg_len);
        }

        ptask->not_received_seg &= ~(1 << sego);
        ptask->block_ack |= (1 << sego);
        ptask->pdu_len += seg_len;
        if (0 == ptask->not_received_seg)
        {
            /* receive all segments */
            meshx_lower_transport_seg_ack(network_if, ptask->block_ack, pmsg_rx_ctx);

            meshx_timer_delete(ptask->ack_timer);
            ptask->ack_timer = NULL;
            meshx_timer_restart(ptask->incomplete_timer, MESHX_LOWER_TRANS_STORE_TIMEOUT);

            /* TODO: notify upper transport layer */
            MESHX_DEBUG("receive upper transport pdu: type %d", pmsg_rx_ctx->ctl);
            MESHX_DUMP_DEBUG(ptask->ppdu, ptask->pdu_len);
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

    return MESHX_SUCCESS;
}

int32_t meshx_lower_transport_receive(meshx_network_if_t network_if, const uint8_t *pdata,
                                      uint8_t len, meshx_msg_rx_ctx_t *pmsg_rx_ctx)
{
    MESHX_DEBUG("receive lower transport data:");
    MESHX_DUMP_DEBUG(pdata, len);
    int32_t ret = MESHX_SUCCESS;
    if (pmsg_rx_ctx->ctl)
    {
        /* control message */
        const meshx_lower_trans_ctl_pdu_metadata_t *pmetadata = (const meshx_lower_trans_ctl_pdu_metadata_t
                                                                 *)pdata;
        if (0 == pmetadata->opcode)
        {
            /* segment ack */
            const meshx_lower_trans_seg_ack_pdu_t *pseg_ack = (const meshx_lower_trans_seg_ack_pdu_t *)(((
                                                                  const meshx_lower_trans_unseg_ctl_pdu_t *)pdata)->pdu);
            meshx_lower_trans_recv_block_ack(pseg_ack);
        }
        else
        {
            /* actual control message */
            if (pmetadata->seg)
            {
                ret = meshx_lower_trans_receive_seg_msg(network_if, pdata, len, pmsg_rx_ctx);
            }
            else
            {
                /* unseg control message */
                const meshx_lower_trans_unseg_ctl_pdu_t *ppdu = (const meshx_lower_trans_unseg_ctl_pdu_t *)pdata;
                uint8_t pdu_len = len - sizeof(meshx_lower_trans_ctl_pdu_metadata_t);

                /* TODO: notify upper transport layer and fill prx_msg_ctx */
                MESHX_DEBUG("receive upper transport pdu: type %d", pmsg_rx_ctx->ctl);
                MESHX_DUMP_DEBUG(ppdu->pdu, pdu_len);
            }
        }
    }
    else
    {
        /* access message */
        const meshx_lower_trans_access_pdu_metadata_t *pmetadata = (const
                                                                    meshx_lower_trans_access_pdu_metadata_t *)pdata;
        if (pmetadata->seg)
        {
            ret = meshx_lower_trans_receive_seg_msg(network_if, pdata, len, pmsg_rx_ctx);
        }
        else
        {
            /* unsegment access message */
            const meshx_lower_trans_unseg_access_pdu_t *ppdu = (const meshx_lower_trans_unseg_access_pdu_t *)
                                                               pdata;
            uint8_t pdu_len = len - sizeof(meshx_lower_trans_access_pdu_metadata_t);

            /* TODO: notify upper transport layer and fill prx_msg_ctx */
            MESHX_DEBUG("receive upper transport pdu: type %d", pmsg_rx_ctx->ctl);
            MESHX_DUMP_DEBUG(ppdu->pdu, pdu_len);
        }
    }

    return ret;
}

