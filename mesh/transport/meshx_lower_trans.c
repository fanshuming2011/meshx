/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_LOWER_TRANS"
#include "meshx_lower_trans.h"
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
#include "meshx_seq.h"
#include "meshx_iv_index.h"
#include "meshx_upper_trans.h"
#include "meshx_rpl.h"
#include "meshx_iv_index_internal.h"
#include "meshx_iv_index.h"

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

#define MESHX_LOWER_TRANS_IS_SEQ_AUTH_VALID(seq, seq_auth)      (((seq) >= (seq_auth)) && (((seq) - (seq_auth)) < 0x2000))
#define MESHX_LOWER_TRANS_SEQ_ZERO(seq)                         ((seq) & 0x1fff)
#define MESHX_LOWER_TRANS_SEQ_AUTH(seq_zero, seq) \
    ((seq_zero) + (((seq) & ~0x1fff) - ((seq_zero) > ((seq) & 0x1fff) ? 0x2000 : 0)))


#define MESHX_LOWER_TRANS_INCOMPLETE_TIMEOUT                    10000 /* ms */
#define MESHX_LOWER_TRANS_RX_ACK_BASE                           150 /* TODO: can configure */
#define MESHX_LOWER_TRANS_RX_ACK_TTL_FACTOR                     50 /* 50 * ttl */ /* TODO: can configure */

#define MESHX_LOWER_TRANS_STORE_TIMEOUT                         10000 /* ms */


/* lower transport tx task */
typedef struct
{
    meshx_msg_ctx_t msg_tx_ctx;
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
    meshx_msg_ctx_t msg_rx_ctx;
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

int32_t meshx_lower_trans_init(void)
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

static bool meshx_lower_trans_rpl_check(meshx_rpl_t rpl)
{
    if (!meshx_rpl_check(rpl))
    {
        return FALSE;
    }
    return (MESHX_SUCCESS == meshx_rpl_update(rpl));
}

static void meshx_lower_trans_tx_timeout_handler(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_TX;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

static int32_t meshx_lower_trans_send_seg_msg(const uint8_t *ppdu,
                                              uint16_t pdu_len, uint32_t block_ack, meshx_msg_ctx_t *pmsg_ctx)
{
    uint8_t seg_num;
    if (pmsg_ctx->ctl)
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

    if (pmsg_ctx->ctl)
    {
        meshx_lower_trans_seg_ctl_pdu_t *pctl_pdu = (meshx_lower_trans_seg_ctl_pdu_t *)pdu;
        pctl_pdu->metadata.opcode = pmsg_ctx->opcode;
        pctl_pdu->metadata.seg = 1;
    }
    else
    {
        meshx_lower_trans_seg_access_pdu_t *paccess_pdu = (meshx_lower_trans_seg_access_pdu_t *)pdu;
        if (pmsg_ctx->akf)
        {
            paccess_pdu->metadata.aid = pmsg_ctx->aid;
        }
        else
        {
            paccess_pdu->metadata.aid = 0;
        }
        paccess_pdu->metadata.akf = pmsg_ctx->akf;
        paccess_pdu->metadata.seg = 1;
    }
    for (uint8_t i = 0; i < seg_num; ++i)
    {
        if (pmsg_ctx->ctl)
        {
            seg_len = (seg_num == (i + 1)) ? (pdu_len - data_offset) :
                      MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE;
        }
        else
        {
            seg_len = (seg_num == (i + 1)) ? (pdu_len - data_offset) :
                      MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
        }

        if (block_ack & (1 << i))
        {
            data_offset += seg_len;
            continue;
        }

        /* use sequence */
        pmsg_ctx->seq = meshx_seq_use(pmsg_ctx->src - meshx_node_params.param.node_addr);

        /* check sequence */
        if (!MESHX_LOWER_TRANS_IS_SEQ_AUTH_VALID(pmsg_ctx->seq, pmsg_ctx->seq_auth))
        {
            MESHX_ERROR("seq is 8192 higher than seq auth: seq 0x%08x, seq auth 0x%08x", pmsg_ctx->seq,
                        pmsg_ctx->seq_auth);
            return -MESHX_ERR_INVAL;
        }

        if (pmsg_ctx->ctl)
        {
            meshx_lower_trans_seg_ctl_pdu_t *pctl_pdu = (meshx_lower_trans_seg_ctl_pdu_t *)pdu;

            pctl_pdu->seg_misc.seq_zero = MESHX_LOWER_TRANS_SEQ_ZERO(pmsg_ctx->seq_auth);
            pctl_pdu->seg_misc.sego = i;
            pctl_pdu->seg_misc.segn = seg_num - 1;
            meshx_swap(pctl_pdu->seg_misc.seg_misc, pctl_pdu->seg_misc.seg_misc + 2);
            memcpy(pctl_pdu->pdu, ppdu + data_offset, seg_len);
        }
        else
        {
            meshx_lower_trans_seg_access_pdu_t *paccess_pdu = (meshx_lower_trans_seg_access_pdu_t *)pdu;

            paccess_pdu->seg_misc.szmic = pmsg_ctx->szmic;
            paccess_pdu->seg_misc.seq_zero = MESHX_LOWER_TRANS_SEQ_ZERO(pmsg_ctx->seq_auth);
            paccess_pdu->seg_misc.sego = i;
            paccess_pdu->seg_misc.segn = seg_num - 1;
            meshx_swap(paccess_pdu->seg_misc.seg_misc, paccess_pdu->seg_misc.seg_misc + 2);
            memcpy(paccess_pdu->pdu, ppdu + data_offset, seg_len);
        }

        data_offset += seg_len;
        if (pmsg_ctx->ctl)
        {
            seg_len += (sizeof(meshx_lower_trans_ctl_pdu_metadata_t) + sizeof(
                            meshx_lower_trans_seg_ctl_misc_t));
        }
        else
        {
            seg_len += (sizeof(meshx_lower_trans_access_pdu_metadata_t) + sizeof(
                            meshx_lower_trans_seg_access_misc_t));
        }
        MESHX_INFO("send segment pdu: %d-%d", i, seg_num);
        MESHX_DUMP_DEBUG(pdu, seg_len);
        meshx_net_send(pdu, seg_len, pmsg_ctx);
    }

    return MESHX_SUCCESS;
}

static void meshx_lower_trans_tx_task_release(meshx_lower_trans_tx_task_t *ptask)
{
    MESHX_ASSERT(NULL != ptask);
    meshx_list_remove(&ptask->node);
    MESHX_INFO("release task(0x%08x)", ptask);
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

    /* check iv index transit state */
    if (meshx_is_iv_update_state_transit_pending())
    {
        meshx_iv_update_state_set(MESHX_IV_UPDATE_STATE_NORMAL);
    }

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

static void meshx_lower_trans_handle_tx_timeout(meshx_lower_trans_tx_task_t *ptask)
{
    ptask->retry_times ++;
    if (ptask->retry_times > meshx_node_params.config.trans_tx_retry_times)
    {
        if (MESHX_ADDRESS_IS_UNICAST(ptask->msg_tx_ctx.dst))
        {
            /* TODO: notify upper transport send failed, no ack from remote or some segment can't be received? */
            MESHX_ERROR("segment message send failed: timeout, seq zero(0x%04x)",
                        MESHX_LOWER_TRANS_SEQ_ZERO(ptask->msg_tx_ctx.seq_auth));
        }
        else
        {
            /* TODO: notify upper transport send finished? */
            MESHX_INFO("segment message send finish: seq zero(0x%04x)",
                       MESHX_LOWER_TRANS_SEQ_ZERO(ptask->msg_tx_ctx.seq_auth));
        }

        meshx_lower_trans_tx_task_finish(ptask);
    }
    else
    {
        /* send segment message */
        meshx_lower_trans_send_seg_msg(ptask->ppdu, ptask->pdu_len, ptask->block_ack,
                                       &ptask->msg_tx_ctx);

        if (!MESHX_LOWER_TRANS_IS_SEQ_AUTH_VALID(ptask->msg_tx_ctx.seq, ptask->msg_tx_ctx.seq_auth))
        {
            /* TODO: notify upper transport send failed, seq is 8192 higher than seq origin? */
            MESHX_ERROR("segment message send failed: seq(0x%06x) is 8192 higher than seq auth(0x%06x)",
                        ptask->msg_tx_ctx.seq, ptask->msg_tx_ctx.seq_auth);
            meshx_lower_trans_tx_task_finish(ptask);
        }
        else
        {
            /* restart timer */
            meshx_lower_trans_tx_timer_start(ptask);
        }
    }
}

void meshx_lower_trans_async_handle_tx_timeout(meshx_async_msg_t msg)
{
    meshx_lower_trans_handle_tx_timeout(msg.pdata);
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

    /* send segment message for the first time */
    int32_t ret;
    ret = meshx_lower_trans_send_seg_msg(ptx_task->ppdu, ptx_task->pdu_len, ptx_task->block_ack,
                                         &ptx_task->msg_tx_ctx);
    if (MESHX_SUCCESS != ret)
    {
        meshx_lower_trans_tx_task_finish(ptx_task);
    }
    else
    {
        /* start retrans timer */
        meshx_lower_trans_tx_timer_start(ptx_task);
    }

    return ret;
}

static int32_t meshx_lower_trans_tx_task_try(meshx_lower_trans_tx_task_t *ptx_task)
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
            meshx_list_append(&meshx_lower_trans_tx_task_pending, &ptx_task->node);
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

    MESHX_INFO("request lower trans task(0x%08x)", ptask);
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

static bool meshx_lower_trans_is_tx_task_exist(const meshx_msg_ctx_t *pmsg_ctx)
{
    meshx_list_t *pnode;
    meshx_lower_trans_tx_task_t *pcur_task;
    meshx_list_foreach(pnode, &meshx_lower_trans_tx_task_active)
    {
        pcur_task = MESHX_CONTAINER_OF(pnode, meshx_lower_trans_tx_task_t, node);
        if ((pcur_task->msg_tx_ctx.dst == pmsg_ctx->dst) &&
            (pcur_task->msg_tx_ctx.seq_auth == pmsg_ctx->seq_auth))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static int32_t meshx_lower_trans_process_seg_msg(const uint8_t *pupper_trans_pdu,
                                                 uint16_t pdu_len, uint8_t max_seg_size, const meshx_msg_ctx_t *pmsg_tx_ctx)
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
                    MESHX_LOWER_TRANS_SEQ_ZERO(pmsg_tx_ctx->seq_auth));
        return -MESHX_ERR_ALREADY;
    }

    /* store segment message for retransmit */
    meshx_lower_trans_tx_task_t *ptask = meshx_lower_trans_tx_task_request(pdu_len);
    if (NULL == ptask)
    {
        MESHX_ERROR("lower transport is busy now, try again later!");
        return -MESHX_ERR_BUSY;
    }
    ptask->msg_tx_ctx = *pmsg_tx_ctx;
    memcpy(ptask->ppdu, pupper_trans_pdu, pdu_len);
    ptask->pdu_len = pdu_len;
    for (uint8_t i = 0; i < seg_num; ++i)
    {
        ptask->seg_bits |= (1 << i);
    }

    return meshx_lower_trans_tx_task_try(ptask);
}

int32_t meshx_lower_trans_send(const uint8_t *pupper_trans_pdu,
                               uint16_t pdu_len, meshx_msg_ctx_t *pmsg_tx_ctx)
{
#if MESHX_REDUNDANCY_CHECK
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
#endif

    int32_t ret = MESHX_SUCCESS;
    if (pmsg_tx_ctx->ctl)
    {
        /* control message */
        if (pmsg_tx_ctx->seg || (pdu_len > MESHX_LOWER_TRANS_UNSEG_CTL_MAX_PDU_SIZE))
        {
            pmsg_tx_ctx->seg = 1;
            /* segment message */
            ret = meshx_lower_trans_process_seg_msg(pupper_trans_pdu, pdu_len,
                                                    MESHX_LOWER_TRANS_SEG_CTL_MAX_PDU_SIZE, pmsg_tx_ctx);
        }
        else
        {
            meshx_lower_trans_unseg_ctl_pdu_t pdu;
            pdu.metadata.opcode = pmsg_tx_ctx->opcode;
            pdu.metadata.seg = 0;
            memcpy(pdu.pdu, pupper_trans_pdu, pdu_len);
            ret = meshx_net_send((const uint8_t *)&pdu, sizeof(meshx_lower_trans_ctl_pdu_metadata_t) + pdu_len,
                                 pmsg_tx_ctx);
        }
    }
    else
    {
        /* access message */
        if (pmsg_tx_ctx->seg || (pdu_len > MESHX_LOWER_TRANS_UNSEG_ACCESS_MAX_PDU_SIZE))
        {
            pmsg_tx_ctx->seg = 1;
            /* segment message */
            ret = meshx_lower_trans_process_seg_msg(pupper_trans_pdu, pdu_len,
                                                    MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE, pmsg_tx_ctx);
        }
        else
        {
            /* unsegment message */
            meshx_lower_trans_unseg_access_pdu_t pdu;
            if (pmsg_tx_ctx->akf)
            {
                pdu.metadata.aid = pmsg_tx_ctx->aid;
            }
            else
            {
                pdu.metadata.aid = 0;
            }
            pdu.metadata.akf = pmsg_tx_ctx->akf;
            pdu.metadata.seg = 0;
            memcpy(pdu.pdu, pupper_trans_pdu, pdu_len);
            ret = meshx_net_send((const uint8_t *)&pdu,
                                 sizeof(meshx_lower_trans_access_pdu_metadata_t) + pdu_len, pmsg_tx_ctx);
        }
    }

    return ret;
}

static int32_t meshx_lower_trans_seg_ack(uint32_t block_ack, const meshx_msg_ctx_t *pmsg_rx_ctx)
{
    meshx_lower_trans_seg_ack_pdu_t seg_ack;
    seg_ack.rfu = 0;
    seg_ack.seq_zero = MESHX_LOWER_TRANS_SEQ_ZERO(pmsg_rx_ctx->seq_auth);
    seg_ack.obo = 0;
    seg_ack.ack_misc = (seg_ack.ack_misc >> 8) | (seg_ack.ack_misc << 8);
    seg_ack.block_ack = MESHX_HOST_TO_BE32(block_ack);

    meshx_msg_ctx_t msg_tx_ctx;
    memset(&msg_tx_ctx, 0, sizeof(msg_tx_ctx));
    msg_tx_ctx.src = pmsg_rx_ctx->dst;
    msg_tx_ctx.dst = pmsg_rx_ctx->src;
    msg_tx_ctx.seg = 0;
    msg_tx_ctx.ttl = meshx_node_params.param.default_ttl;
    msg_tx_ctx.ctl = 1;
    msg_tx_ctx.iv_index = meshx_iv_index_tx_get();
    msg_tx_ctx.pnet_key = pmsg_rx_ctx->pnet_key;
    msg_tx_ctx.opcode = 0;
    msg_tx_ctx.seq = meshx_seq_use(msg_tx_ctx.src - meshx_node_params.param.node_addr);

    MESHX_INFO("lower transport block ack: 0x%08x", block_ack);
    return meshx_lower_trans_send((const uint8_t *)&seg_ack,
                                  sizeof(seg_ack), &msg_tx_ctx);
}

static void meshx_lower_trans_rx_task_release(meshx_lower_trans_rx_task_t *ptask)
{
    MESHX_ASSERT(NULL != ptask);
    meshx_list_remove(&ptask->node);
    MESHX_INFO("release rx task: 0x%08x", ptask);

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
    meshx_lower_trans_seg_ack(ptask->block_ack, &ptask->msg_rx_ctx);
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
                                                                      const meshx_msg_ctx_t *pmsg_rx_ctx)
{
    MESHX_ASSERT(NULL != pmsg_rx_ctx);
    /* check exists */
    meshx_list_t *pnode;
    meshx_lower_trans_rx_task_t *prx_task;
    meshx_list_foreach(pnode, &meshx_lower_trans_rx_task_active)
    {
        prx_task = MESHX_CONTAINER_OF(pnode, meshx_lower_trans_rx_task_t, node);
        if (prx_task->msg_rx_ctx.src == pmsg_rx_ctx->src)
        {
            uint64_t seq_auth_rx = meshx_lower_trans_seq_auth(pmsg_rx_ctx->iv_index, pmsg_rx_ctx->seq_auth);
            uint64_t seq_auth_store = meshx_lower_trans_seq_auth(prx_task->msg_rx_ctx.iv_index,
                                                                 prx_task->msg_rx_ctx.seq_auth);
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
                    MESHX_WARN("receive new seg message, cancel old: seq zero 0x%04x",
                               MESHX_LOWER_TRANS_SEQ_ZERO(prx_task->msg_rx_ctx.seq_auth));
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
                MESHX_INFO("receive old segment: iv index 0x%08x, seq zero 0x%04x", pmsg_rx_ctx->iv_index,
                           MESHX_LOWER_TRANS_SEQ_ZERO(pmsg_rx_ctx->seq_auth));
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

    prx_task->msg_rx_ctx = *pmsg_rx_ctx;
    /* initialize segment flag */
    uint8_t seg_num = 0;
    if (pmsg_rx_ctx->ctl)
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
        if (MESHX_LOWER_TRANS_SEQ_ZERO(pcur_task->msg_tx_ctx.seq_auth) == seg_ack.seq_zero)
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
            MESHX_INFO("remote receive all segments, seq zero 0x%04x",
                       MESHX_LOWER_TRANS_SEQ_ZERO(pcur_task->msg_tx_ctx.seq_auth));
            meshx_lower_trans_tx_task_finish(pcur_task);
            /* TODO: notify upper transport send success? */
        }
        else
        {
            if (0 == seg_ack.block_ack)
            {
                MESHX_WARN("send canceled, remote can't receive!");
                meshx_lower_trans_tx_task_finish(pcur_task);
                /* TODO: notify upper transport send canceled, remote can't receive? */
            }
            else
            {
                pcur_task->retry_times ++;
                if (pcur_task->retry_times > meshx_node_params.config.trans_tx_retry_times)
                {
                    meshx_lower_trans_tx_task_finish(pcur_task);
                    if (MESHX_ADDRESS_IS_UNICAST(pcur_task->msg_tx_ctx.dst))
                    {
                        MESHX_ERROR("segment message send failed: seq zero 0x%04x",
                                    MESHX_LOWER_TRANS_SEQ_ZERO(pcur_task->msg_tx_ctx.seq_auth));
                        /* TODO: notify upper transport send failed, some segment can't be received? */
                    }
                }
                else
                {
                    pcur_task->block_ack = seg_ack.block_ack;
                    meshx_lower_trans_send_seg_msg(pcur_task->ppdu, pcur_task->pdu_len, pcur_task->block_ack,
                                                   &pcur_task->msg_tx_ctx);
                    if (!MESHX_LOWER_TRANS_IS_SEQ_AUTH_VALID(pcur_task->msg_tx_ctx.seq,
                                                             pcur_task->msg_tx_ctx.seq_auth))
                    {
                        meshx_lower_trans_tx_task_finish(pcur_task);
                        /* TODO: notify upper transport send failed, seq is 8192 higher than seq origin? */
                    }
                    else
                    {
                        /* restart retrans timer */
                        meshx_lower_trans_tx_timer_start(pcur_task);
                    }
                }
            }
        }
    }
    else
    {
        MESHX_WARN("receive invalid segment ack: seq zero 0x%04x", seg_ack.seq_zero);
    }
}

static int32_t meshx_lower_trans_receive_seg_msg(const uint8_t *pdata,
                                                 uint8_t len, meshx_msg_ctx_t *pmsg_rx_ctx)
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
        pmsg_rx_ctx->seq_auth = MESHX_LOWER_TRANS_SEQ_AUTH(seg_misc.seq_zero, pmsg_rx_ctx->seq);
        pmsg_rx_ctx->opcode = pseg_ctl_msg->metadata.opcode;
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
        pmsg_rx_ctx->seq_auth = MESHX_LOWER_TRANS_SEQ_AUTH(seg_misc.seq_zero, pmsg_rx_ctx->seq);
        pmsg_rx_ctx->akf = pseg_access_msg->metadata.akf;
        pmsg_rx_ctx->aid = pseg_access_msg->metadata.aid;
        pmsg_rx_ctx->szmic = seg_misc.szmic;
        sego = seg_misc.sego;
        segn = seg_misc.segn;
        max_pdu_len = (seg_misc.segn + 1) * MESHX_LOWER_TRANS_SEG_ACCESS_MAX_PDU_SIZE;
    }

    meshx_lower_trans_rx_task_t *ptask = meshx_lower_trans_rx_task_request(max_pdu_len, pmsg_rx_ctx);

    if (NULL == ptask)
    {
        MESHX_ERROR("lower trans rx task run out!");
        meshx_lower_trans_seg_ack(0, pmsg_rx_ctx);
        return -MESHX_ERR_BUSY;
    }

    /* check parameters */
    if (pmsg_rx_ctx->ctl)
    {
        if (ptask->msg_rx_ctx.opcode != pmsg_rx_ctx->opcode)
        {
            MESHX_ERROR("received opcode(%d) mismatch with previous(%d)", pmsg_rx_ctx->opcode,
                        ptask->msg_rx_ctx.opcode);
            /* ack can't receive */
            meshx_lower_trans_seg_ack(0, pmsg_rx_ctx);
            /* release rx task */
            meshx_lower_trans_rx_task_release(ptask);
            return -MESHX_ERR_INVAL;
        }
    }
    else
    {
        if ((ptask->msg_rx_ctx.akf != pmsg_rx_ctx->akf) ||
            (ptask->msg_rx_ctx.aid != pmsg_rx_ctx->aid) ||
            (ptask->msg_rx_ctx.szmic != pmsg_rx_ctx->szmic))
        {
            MESHX_ERROR("received akf-aid-szmic(%d-%d-%d) mismatch with previous(%d-%d-%d)", pmsg_rx_ctx->akf,
                        pmsg_rx_ctx->aid, pmsg_rx_ctx->szmic, ptask->msg_rx_ctx.akf, ptask->msg_rx_ctx.aid,
                        ptask->msg_rx_ctx.szmic);
            /* ack can't receive */
            meshx_lower_trans_seg_ack(0, pmsg_rx_ctx);
            /* release rx task */
            meshx_lower_trans_rx_task_release(ptask);
            return -MESHX_ERR_INVAL;
        }
    }

    if (ptask->max_pdu_len != max_pdu_len)
    {
        MESHX_ERROR("receive segment length dismatch with previous: %d-%d", max_pdu_len,
                    ptask->max_pdu_len);
        /* ack can't receive */
        meshx_lower_trans_seg_ack(0, pmsg_rx_ctx);
        /* release rx task */
        meshx_lower_trans_rx_task_release(ptask);
        return -MESHX_ERR_LENGTH;
    }

    if (0 == ptask->not_received_seg)
    {
        MESHX_INFO("already received all segments");
        /* update rpl */
        meshx_rpl_t rpl = {pmsg_rx_ctx->src, pmsg_rx_ctx->seq, pmsg_rx_ctx->iv_index};
        meshx_rpl_update(rpl);
        /* ack immedietely */
        meshx_lower_trans_seg_ack(ptask->block_ack, pmsg_rx_ctx);
        return -MESHX_ERR_ALREADY;
    }

    if (0 == (ptask->not_received_seg & (1 << sego)))
    {
        MESHX_INFO("seg %d already received", (1 << sego) - 1);
        return -MESHX_ERR_ALREADY;
    }

    if (0 == segn)
    {
        /* only one segment */
        meshx_rpl_t rpl = {pmsg_rx_ctx->src, pmsg_rx_ctx->seq, pmsg_rx_ctx->iv_index};
        if (!meshx_lower_trans_rpl_check(rpl))
        {
            /* ack can't receive */
            meshx_lower_trans_seg_ack(0, pmsg_rx_ctx);
            /* release rx task */
            meshx_lower_trans_rx_task_release(ptask);
            return -MESHX_ERR_FAIL;
        }

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
        meshx_lower_trans_seg_ack(ptask->block_ack, pmsg_rx_ctx);

        /* stop timer */
        meshx_timer_delete(ptask->ack_timer);
        ptask->ack_timer = NULL;
        meshx_timer_restart(ptask->incomplete_timer, MESHX_LOWER_TRANS_STORE_TIMEOUT);

        /* notify upper transport layer */
        pmsg_rx_ctx->seg = 1;
        meshx_upper_trans_receive(ptask->ppdu, ptask->pdu_len, pmsg_rx_ctx);
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
            meshx_rpl_t rpl = {pmsg_rx_ctx->src, pmsg_rx_ctx->seq, pmsg_rx_ctx->iv_index};
            if (!meshx_lower_trans_rpl_check(rpl))
            {
                /* ack can't receive */
                meshx_lower_trans_seg_ack(0, pmsg_rx_ctx);
                /* release rx task */
                meshx_lower_trans_rx_task_release(ptask);
                return -MESHX_ERR_FAIL;
            }

            /* receive all segments */
            meshx_lower_trans_seg_ack(ptask->block_ack, pmsg_rx_ctx);

            meshx_timer_delete(ptask->ack_timer);
            ptask->ack_timer = NULL;
            meshx_timer_restart(ptask->incomplete_timer, MESHX_LOWER_TRANS_STORE_TIMEOUT);

            /* notify upper transport layer */
            pmsg_rx_ctx->seg = 1;
            meshx_upper_trans_receive(ptask->ppdu, ptask->pdu_len, pmsg_rx_ctx);
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

int32_t meshx_lower_trans_receive(uint8_t *pdata, uint8_t len, meshx_msg_ctx_t *pmsg_rx_ctx)
{
    MESHX_DEBUG("receive lower transport data:");
    MESHX_DUMP_DEBUG(pdata, len);
    int32_t ret = MESHX_SUCCESS;
    meshx_rpl_t rpl = {pmsg_rx_ctx->src, pmsg_rx_ctx->seq, pmsg_rx_ctx->iv_index};

    if (pmsg_rx_ctx->ctl)
    {
        /* control message */
        meshx_lower_trans_ctl_pdu_metadata_t *pmetadata = (meshx_lower_trans_ctl_pdu_metadata_t *)pdata;

        if (0 == pmetadata->opcode)
        {
            if (!meshx_lower_trans_rpl_check(rpl))
            {
                return -MESHX_ERR_FAIL;
            }

            /* segment ack */
            meshx_lower_trans_seg_ack_pdu_t *pseg_ack = (meshx_lower_trans_seg_ack_pdu_t *)(((
                    meshx_lower_trans_unseg_ctl_pdu_t *)pdata)->pdu);
            meshx_lower_trans_recv_block_ack(pseg_ack);
        }
        else
        {
            /* actual control message */
            if (pmetadata->seg)
            {
                ret = meshx_lower_trans_receive_seg_msg(pdata, len, pmsg_rx_ctx);
            }
            else
            {
                if (!meshx_lower_trans_rpl_check(rpl))
                {
                    return -MESHX_ERR_FAIL;
                }

                /* unseg control message */
                meshx_lower_trans_unseg_ctl_pdu_t *ppdu = (meshx_lower_trans_unseg_ctl_pdu_t *)pdata;
                uint8_t pdu_len = len - sizeof(meshx_lower_trans_ctl_pdu_metadata_t);

                /* notify upper transport layer and fill prx_msg_ctx */
                pmsg_rx_ctx->seg = 0;
                pmsg_rx_ctx->opcode = pmetadata->opcode;
                pmsg_rx_ctx->seq_auth = pmsg_rx_ctx->seq;
                meshx_upper_trans_receive(ppdu->pdu, pdu_len, pmsg_rx_ctx);
            }
        }
    }
    else
    {
        /* access message */
        meshx_lower_trans_access_pdu_metadata_t *pmetadata = (meshx_lower_trans_access_pdu_metadata_t *)
                                                             pdata;
        if (pmetadata->seg)
        {
            ret = meshx_lower_trans_receive_seg_msg(pdata, len, pmsg_rx_ctx);
        }
        else
        {
            if (!meshx_lower_trans_rpl_check(rpl))
            {
                return -MESHX_ERR_FAIL;
            }

            /* unsegment access message */
            meshx_lower_trans_unseg_access_pdu_t *ppdu = (meshx_lower_trans_unseg_access_pdu_t *)pdata;
            uint8_t pdu_len = len - sizeof(meshx_lower_trans_access_pdu_metadata_t);

            /* notify upper transport layer and fill prx_msg_ctx */
            pmsg_rx_ctx->akf = pmetadata->akf;
            pmsg_rx_ctx->aid = pmetadata->aid;
            pmsg_rx_ctx->seg = 0;
            pmsg_rx_ctx->szmic = 0;
            pmsg_rx_ctx->seq_auth = pmsg_rx_ctx->seq;
            meshx_upper_trans_receive(ppdu->pdu, pdu_len, pmsg_rx_ctx);
        }
    }

    return ret;
}

bool meshx_is_lower_trans_busy(void)
{
    return (meshx_lower_trans_tx_task_active.pnext != &meshx_lower_trans_tx_task_active);
}
