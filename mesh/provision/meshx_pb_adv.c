/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx_config.h"
#define MESHX_TRACE_MODULE "MESHX_PB_ADV"
#include "meshx_trace.h"
#include "meshx_gap.h"
#include "meshx_pb_adv.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_list.h"
#include "meshx_node.h"
#include "meshx_3gpp_crc.h"
#include "meshx_timer.h"
#include "meshx_misc.h"
#include "meshx_endianness.h"
#include "meshx_bit_field.h"
#include "meshx_prov_internal.h"
#include "meshx_bearer_internal.h"
#include "meshx_async_internal.h"
#include "meshx_notify.h"
#include "meshx_notify_internal.h"
#include "meshx_node_internal.h"


/* maximum trans segment number */
#define MESHX_TRANS_SEG_NUM_MAX                        0x40

#define MESHX_LINK_LOSS_TIME                           600000 /* unit is ms */
#define MESHX_LINK_OPEN_RETRY_PERIOD                   200 /* unit is ms */
#define MESHX_LINK_CLOSE_RETRY_PERIOD                  20 /* unit is ms */
#define MESHX_LINK_CLOSE_TIME                          90 /* unit is ms */

#define MESHX_TRANS_LOSS_TIME                          30000 /* unit is ms */
#define MESHX_TRANS_RETRY_PERIOD                       300 /* unit is ms */

#define MESHX_BEARER_CTL_TRANS_NUM                     0

#define MESHX_IS_VALID_DEVICE_TRANS_NUM(num)           (((num) >= 0x80) && ((num) <= 0xff))
#define MESHX_IS_VALID_PROVISIONER_TRANS_NUM(num)      (((num) >= 0) && ((num) <= 0x7f))

#define MESHX_BEARER_CTL_METADATA_LEN                  (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t))
#define MESHX_LINK_OPEN_PDU_LEN                        (MESHX_BEARER_CTL_METADATA_LEN + sizeof(meshx_pb_adv_link_open_t))
#define MESHX_LINK_ACK_PDU_LEN                         (MESHX_BEARER_CTL_METADATA_LEN + sizeof(meshx_pb_adv_link_ack_t))
#define MESHX_LINK_CLOSE_PDU_LEN                       (MESHX_BEARER_CTL_METADATA_LEN + sizeof(meshx_pb_adv_link_close_t))
#define MESHX_TRANS_ACK_PDU_LEN                        (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_ack_metadata_t))

#define MESHX_TRANS_START_METADATA_LEN                 (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_start_metadata_t))
#define MESHX_TRANS_START_DATA_MAX_LEN                 (MESHX_PB_ADV_PDU_MAX_LEN - MESHX_TRANS_START_METADATA_LEN)
#define MESHX_TRANS_CONTINUE_METADATA_LEN              (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_continue_metadata_t))
#define MESHX_TRANS_CONTINUE_DATA_MAX_LEN              (MESHX_PB_ADV_PDU_MAX_LEN - MESHX_TRANS_CONTINUE_METADATA_LEN)

#define MESHX_TRANS_DATA_MAX_LEN                       (MESHX_TRANS_START_DATA_MAX_LEN + MESHX_TRANS_SEG_NUM_MAX * MESHX_TRANS_CONTINUE_DATA_MAX_LEN)

#define MESHX_TRANS_DATA_VALID_MAX_LEN                 65

#define MESHX_LINK_CLOSE_REPEAT_TIMES                  3

#define MESHX_PB_ADV_TIMER_STATE_DELAY                 0
#define MESHX_PB_ADV_TIMER_STATE_LINK_MONITOR          1
#define MESHX_PB_ADV_TIMER_STATE_RETRY                 2

#define MESHX_PB_ADV_TIMER_DELAY_TYPE_TRANS_ACK        0
#define MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU         1
typedef struct
{
    struct _meshx_prov_dev dev;

    uint32_t link_id;

    uint8_t tx_trans_num;
    uint8_t rx_trans_num;
    uint8_t acked_trans_num;

    uint32_t retry_time;
    meshx_timer_t pb_adv_timer;
    uint8_t timer_state;
    uint8_t timer_delay_type;

    uint8_t link_close_reason;

    uint8_t last_seg_num;
    uint8_t fcs;
    uint8_t recv_seg[MESHX_TRANS_SEG_NUM_MAX / 8];
    uint16_t trans_data_len; /* 0: no trans data has been received >0: trans data length */
    union
    {
        meshx_prov_pdu_t pdu;
        uint8_t data[MESHX_TRANS_DATA_VALID_MAX_LEN];
    } prov_rx_pdu;

    meshx_list_t node;
} meshx_pb_adv_dev_t;

static meshx_list_t pb_adv_devs;

int32_t meshx_pb_adv_init(void)
{
    meshx_list_init_head(&pb_adv_devs);
    return MESHX_SUCCESS;
}

static uint8_t meshx_pb_adv_rand(void)
{
    uint32_t val = MESHX_ABS(meshx_rand());
    val %= 50;
    if (val < 20)
    {
        val += 20;
    }

    return val;
}

static uint8_t meshx_calc_seg_num(uint16_t len)
{
    uint8_t segment_num = 0;
    if (len <= MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN)
    {
        segment_num = 1;
    }
    else
    {
        segment_num = ((len - MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN) +
                       MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN - 1) / MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN + 1;
    }

    return segment_num;
}

static meshx_prov_dev_t meshx_pb_adv_find_device(meshx_bearer_t bearer,
                                                 meshx_dev_uuid_t dev_uuid)
{
    meshx_list_t *pnode = NULL;
    meshx_pb_adv_dev_t *pdev = NULL;
    meshx_list_foreach(pnode, &pb_adv_devs)
    {
        pdev = MESHX_CONTAINER_OF(pnode, meshx_pb_adv_dev_t, node);
        if ((0 == memcmp(pdev->dev.dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t))) &&
            (pdev->dev.bearer == bearer))
        {
            return &pdev->dev;
        }
    }

    return NULL;
}

void meshx_pb_adv_delete_device(meshx_prov_dev_t prov_dev)
{
    if (NULL == prov_dev)
    {
        return ;
    }
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    meshx_list_remove(&pdev->node);
    if (NULL != pdev->pb_adv_timer)
    {
        meshx_timer_delete(pdev->pb_adv_timer);
    }

    meshx_free(pdev);
}

static bool meshx_pb_adv_is_device_valid(meshx_pb_adv_dev_t *pdev)
{
    meshx_list_t *pnode = NULL;
    meshx_pb_adv_dev_t *ptmp_dev = NULL;
    meshx_list_foreach(pnode, &pb_adv_devs)
    {
        ptmp_dev = MESHX_CONTAINER_OF(pnode, meshx_pb_adv_dev_t, node);
        if (ptmp_dev == pdev)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static int32_t pb_adv_send_trans(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                                 const uint8_t *pdata, uint16_t len)
{
    int32_t ret = MESHX_SUCCESS;
    uint8_t segment_num = meshx_calc_seg_num(len);
    MESHX_ASSERT(segment_num  <= MESHX_TRANS_SEG_NUM_MAX);

    meshx_pb_adv_pkt_t pb_adv_pkt;
    uint8_t segment_len = 0;
    uint8_t data_offset = 0;
    for (uint8_t index = 0; index < segment_num; ++index)
    {
        if (0 == index)
        {
            /* trans start*/
            pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
            pb_adv_pkt.metadata.trans_num = trans_num;
            pb_adv_pkt.trans_start.metadata.gpcf = MESHX_GPCF_TRANS_START;
            pb_adv_pkt.trans_start.metadata.last_seg_num = segment_num - 1;
            pb_adv_pkt.trans_start.metadata.total_len = MESHX_HOST_TO_BE16(len);
            pb_adv_pkt.trans_start.metadata.fcs = meshx_3gpp_crc(pdata, len);
            segment_len = (segment_num == 1) ? len : MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN;
            memcpy(pb_adv_pkt.trans_start.pdu, pdata, segment_len);
            data_offset += segment_len;
            segment_len += sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_start_metadata_t);
        }
        else
        {
            /* trans continue */
            pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
            pb_adv_pkt.metadata.trans_num = trans_num;
            pb_adv_pkt.trans_continue.metadata.gpcf = MESHX_GPCF_TRANS_CONTINUE;
            pb_adv_pkt.trans_continue.metadata.seg_index = index;
            segment_len = (segment_num == (index + 1)) ? (len - data_offset) :
                          MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN;
            memcpy(pb_adv_pkt.trans_continue.pdu, pdata + data_offset, segment_len);
            data_offset += segment_len;
            segment_len += sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_continue_metadata_t);
        }
        ret = meshx_bearer_adv_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                                    segment_len);
        if (MESHX_SUCCESS != ret)
        {
            break;
        }
    }

    return ret;
}

#if MESHX_SUPPORT_ROLE_PROVISIONER
static int32_t pb_adv_link_open(meshx_bearer_t bearer, uint32_t link_id, meshx_dev_uuid_t dev_uuid)
{
    MESHX_INFO("link opening: %d", link_id);

    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_OPEN;
    memcpy(pb_adv_pkt.bearer_ctl.link_open.dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t));
    return meshx_bearer_adv_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                                 MESHX_LINK_OPEN_PDU_LEN);
}
#endif

#if MESHX_SUPPORT_ROLE_DEVICE
static int32_t pb_adv_link_ack(meshx_bearer_t bearer, uint32_t link_id)
{
    MESHX_INFO("link ack: %d", link_id);

    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_ACK;
    return meshx_bearer_adv_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV,
                                 (const uint8_t *)&pb_adv_pkt, MESHX_LINK_ACK_PDU_LEN);
}
#endif

static int32_t pb_adv_link_close(meshx_bearer_t bearer, uint32_t link_id, uint8_t reason)
{
    MESHX_INFO("link close: %d", link_id);

    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_CLOSE;
    pb_adv_pkt.bearer_ctl.link_close.reason = reason;
    return meshx_bearer_adv_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                                 MESHX_LINK_CLOSE_PDU_LEN);
}

static int32_t pb_adv_trans_ack(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num)
{
    MESHX_INFO("trans ack: %d", trans_num);

    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = trans_num;
    pb_adv_pkt.trans_ack.metadata.gpcf = MESHX_GPCF_TRANS_ACK;
    pb_adv_pkt.trans_ack.metadata.padding = 0;
    return meshx_bearer_adv_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                                 MESHX_TRANS_ACK_PDU_LEN);
}

#if MESHX_SUPPORT_ROLE_PROVISIONER
static int32_t pb_adv_invite(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                             meshx_prov_invite_t invite)
{
    MESHX_INFO("invite: %d", invite.attention_duration);

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_INVITE;
    prov_pdu.metadata.padding = 0;
    prov_pdu.invite = invite;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t) + sizeof(meshx_prov_invite_t));
}
#endif

#if MESHX_SUPPORT_ROLE_DEVICE
static int32_t pb_adv_capabilites(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                                  const meshx_prov_capabilites_t *pcap)
{
    MESHX_INFO("capabilites:");
    MESHX_DUMP_DEBUG(pcap, sizeof(meshx_prov_capabilites_t));

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_CAPABILITES;
    prov_pdu.metadata.padding = 0;
    prov_pdu.capabilites = *pcap;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t) + sizeof(meshx_prov_capabilites_t));
}
#endif

#if MESHX_SUPPORT_ROLE_PROVISIONER
static int32_t pb_adv_start(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                            const meshx_prov_start_t *pstart)
{
    MESHX_INFO("start:");
    MESHX_DUMP_DEBUG(pstart, sizeof(meshx_prov_start_t));

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_START;
    prov_pdu.metadata.padding = 0;
    prov_pdu.start = *pstart;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t) + sizeof(meshx_prov_start_t));
}
#endif

static int32_t pb_adv_public_key(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                                 const meshx_prov_public_key_t *ppub_key)
{
    MESHX_INFO("public key:");
    MESHX_DUMP_DEBUG(ppub_key, sizeof(meshx_prov_public_key_t));

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_PUBLIC_KEY;
    prov_pdu.metadata.padding = 0;
    prov_pdu.public_key = *ppub_key;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t) + sizeof(meshx_prov_public_key_t));
}

#if MESHX_SUPPORT_ROLE_PROVISIONER
static int32_t pb_adv_input_complete(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num)
{
    MESHX_INFO("input complete");

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_INPUT_COMPLETE;
    prov_pdu.metadata.padding = 0;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t));
}
#endif

static int32_t pb_adv_confirmation(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                                   const meshx_prov_confirmation_t *pcfm)
{
    MESHX_INFO("confirmation:");
    MESHX_DUMP_DEBUG(pcfm, sizeof(meshx_prov_confirmation_t));

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_CONFIRMATION;
    prov_pdu.metadata.padding = 0;
    prov_pdu.confirmation = *pcfm;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t) + sizeof(meshx_prov_confirmation_t));
}

static int32_t pb_adv_random(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                             const meshx_prov_random_t *prandom)
{
    MESHX_INFO("random:");
    MESHX_DUMP_DEBUG(prandom, sizeof(meshx_prov_random_t));

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_RANDOM;
    prov_pdu.metadata.padding = 0;
    prov_pdu.random = *prandom;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t) + sizeof(meshx_prov_random_t));
}

#if MESHX_SUPPORT_ROLE_PROVISIONER
static int32_t pb_adv_data(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                           const meshx_prov_data_t *pdata)
{
    MESHX_INFO("data:");
    MESHX_DUMP_DEBUG(pdata, sizeof(meshx_prov_data_t));

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_DATA;
    prov_pdu.metadata.padding = 0;
    prov_pdu.data = *pdata;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t) + sizeof(meshx_prov_data_t));
}
#endif

#if MESHX_SUPPORT_ROLE_DEVICE
static int32_t pb_adv_complete(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num)
{
    MESHX_INFO("complete");

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_COMPLETE;
    prov_pdu.metadata.padding = 0;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t));
}

static int32_t pb_adv_failed(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                             uint8_t err_code)
{
    MESHX_INFO("error code: %d", err_code);

    meshx_prov_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROV_TYPE_FAILED;
    prov_pdu.metadata.padding = 0;
    prov_pdu.err_code = err_code;
    return pb_adv_send_trans(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_prov_pdu_metadata_t) + sizeof(uint8_t));
}
#endif

static void meshx_pb_adv_handle_delay(meshx_pb_adv_dev_t *pdev)
{
    MESHX_INFO("delay timeout: %d", pdev->timer_delay_type);
    switch (pdev->timer_delay_type)
    {
    case MESHX_PB_ADV_TIMER_DELAY_TYPE_TRANS_ACK:
        {
            int32_t ret = pb_adv_trans_ack(pdev->dev.bearer, pdev->link_id, pdev->rx_trans_num);
            if (MESHX_SUCCESS == ret)
            {
                pdev->acked_trans_num = pdev->rx_trans_num;
            }

            /* change timer period to monitor link */
            pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_LINK_MONITOR;
            meshx_timer_start(pdev->pb_adv_timer, MESHX_LINK_LOSS_TIME);

            /* process provision pdu */
            uint16_t data_len = pdev->trans_data_len;
            /* clear trans data to enable receive new trans packet */
            pdev->trans_data_len = 0;
            /* check trans num */
            if (MESHX_ROLE_DEVICE == pdev->dev.role)
            {
                if (!MESHX_IS_VALID_PROVISIONER_TRANS_NUM(pdev->rx_trans_num))
                {
                    /* invalid trans num */
                    pdev->dev.err_code = MESHX_PROV_FAILED_UNEXPECTED_ERROR;
                    meshx_pb_adv_failed(&pdev->dev);
                    return ;
                }
            }
            else
            {
                if (!MESHX_IS_VALID_DEVICE_TRANS_NUM(pdev->rx_trans_num))
                {
                    /* invalid trans num */
                    pdev->dev.err_code = MESHX_PROV_FAILED_UNEXPECTED_ERROR;
                    meshx_pb_adv_failed(&pdev->dev);
                    return ;
                }
            }

            meshx_prov_pdu_process(&pdev->dev, pdev->prov_rx_pdu.data,
                                   data_len);
        }
        break;
    case MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU:
        switch (pdev->dev.state)
        {
        case MESHX_PROV_STATE_INVITE:
            pb_adv_invite(pdev->dev.bearer, pdev->link_id,
                          pdev->tx_trans_num, pdev->dev.invite);
            break;
        case MESHX_PROV_STATE_CAPABILITES:
            pb_adv_capabilites(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num,
                               &pdev->dev.capabilites);
            break;
        case MESHX_PROV_STATE_START:
            pb_adv_start(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num,
                         &pdev->dev.start);
            break;
        case MESHX_PROV_STATE_PUBLIC_KEY:
            pb_adv_public_key(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num,
                              (const meshx_prov_public_key_t *)&pdev->dev.public_key);
            break;
        case MESHX_PROV_STATE_INPUT_COMPLETE:
            pb_adv_input_complete(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num);
            break;
        case MESHX_PROV_STATE_CONFIRMATION:
            pb_adv_confirmation(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num, &pdev->dev.confirmation);
            break;
        case MESHX_PROV_STATE_RANDOM:
            pb_adv_random(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num, &pdev->dev.random);
            break;
        case MESHX_PROV_STATE_DATA:
            pb_adv_data(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num, &pdev->dev.data);
            break;
        case MESHX_PROV_STATE_COMPLETE:
            pb_adv_complete(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num);
            break;
        case MESHX_PROV_STATE_FAILED:
            pb_adv_failed(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num, pdev->dev.err_code);
            break;
        default:
            /* should never reach here */
            MESHX_ASSERT(0);
            break;
        }

        /* change timer period to retry */
        pdev->retry_time = 0;
        pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_RETRY;
        meshx_timer_start(pdev->pb_adv_timer, MESHX_TRANS_RETRY_PERIOD);
        break;
    default:
        MESHX_ERROR("invalid delay type: %d", pdev->timer_delay_type);
        break;
    }
}

static void meshx_pb_adv_handle_link_loss(meshx_pb_adv_dev_t *pdev)
{
    MESHX_WARN("link loss in state: %d", pdev->dev.state);
    /* stop pb adv timer */
    if (NULL != pdev->pb_adv_timer)
    {
        meshx_timer_stop(pdev->pb_adv_timer);
    }

    meshx_pb_adv_link_close(&pdev->dev, MESHX_PROV_LINK_CLOSE_LINK_LOSS);
}

static void meshx_pb_adv_handle_retry(meshx_pb_adv_dev_t *pdev)
{
    if (MESHX_PROV_STATE_LINK_OPENING == pdev->dev.state)
    {
        pb_adv_link_open(pdev->dev.bearer, pdev->link_id, pdev->dev.dev_uuid);
        pdev->retry_time += MESHX_LINK_OPEN_RETRY_PERIOD;
        if (pdev->retry_time > MESHX_LINK_LOSS_TIME)
        {
            MESHX_WARN("provision failed: receive no link ack from device uuid:");
            MESHX_DUMP_ERROR(pdev->dev.dev_uuid, sizeof(meshx_dev_uuid_t));

            /* stop pb adv timer */
            if (NULL != pdev->pb_adv_timer)
            {
                meshx_timer_stop(pdev->pb_adv_timer);
            }

            /* notify app link open failed, timeout */
            meshx_prov_link_open_result_t result = MESHX_PROV_LINK_OPEN_TIMEOUT;
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = &pdev->dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_LINK_OPEN;
            notify_prov.pdata = &result;
            meshx_prov_handle_notify(pdev->dev.bearer, &notify_prov,
                                     sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_prov_link_open_result_t));
        }
    }
    else if (MESHX_PROV_STATE_LINK_CLOSING == pdev->dev.state)
    {
        uint8_t reason = pdev->link_close_reason;
        if (MESHX_PROV_LINK_CLOSE_LINK_LOSS == pdev->link_close_reason)
        {
            reason = MESHX_LINK_CLOSE_REASON_TIMEOUT;
        }
        pb_adv_link_close(pdev->dev.bearer, pdev->link_id, reason);
        pdev->retry_time += MESHX_LINK_CLOSE_RETRY_PERIOD;
        if (pdev->retry_time > MESHX_LINK_CLOSE_TIME)
        {
            MESHX_WARN("link closed: id %d reason %d", pdev->link_id, pdev->link_close_reason);

            /* stop pb adv timer */
            if (NULL != pdev->pb_adv_timer)
            {
                meshx_timer_stop(pdev->pb_adv_timer);
            }

            /* notify app link close */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = &pdev->dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_LINK_CLOSE;
            notify_prov.pdata = &pdev->link_close_reason;
            meshx_prov_handle_notify(pdev->dev.bearer, &notify_prov,
                                     sizeof(meshx_notify_prov_metadata_t) + sizeof(uint8_t));
        }
    }
    else if ((pdev->dev.state >= MESHX_PROV_STATE_INVITE) &&
             (pdev->dev.state <= MESHX_PROV_STATE_COMPLETE))
    {
        switch (pdev->dev.state)
        {
        case MESHX_PROV_STATE_INVITE:
            pb_adv_invite(pdev->dev.bearer, pdev->link_id,
                          pdev->tx_trans_num, pdev->dev.invite);
            break;
        case MESHX_PROV_STATE_CAPABILITES:
            pb_adv_capabilites(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num,
                               &pdev->dev.capabilites);
            break;
        case MESHX_PROV_STATE_START:
            pb_adv_start(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num,
                         &pdev->dev.start);
            break;
        case MESHX_PROV_STATE_PUBLIC_KEY:
            pb_adv_public_key(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num,
                              (const meshx_prov_public_key_t *)&pdev->dev.public_key);
            break;
        case MESHX_PROV_STATE_INPUT_COMPLETE:
            pb_adv_input_complete(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num);
            break;
        case MESHX_PROV_STATE_CONFIRMATION:
            pb_adv_confirmation(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num, &pdev->dev.confirmation);
            break;
        case MESHX_PROV_STATE_RANDOM:
            pb_adv_random(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num, &pdev->dev.random);
            break;
        case MESHX_PROV_STATE_FAILED:
            pb_adv_failed(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num, pdev->dev.err_code);
            break;
        case MESHX_PROV_STATE_DATA:
            pb_adv_data(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num, &pdev->dev.data);
            break;
        case MESHX_PROV_STATE_COMPLETE:
            pb_adv_complete(pdev->dev.bearer, pdev->link_id, pdev->tx_trans_num);
            break;
        default:
            /* should never reach here */
            MESHX_ASSERT(0);
            break;
        }
        pdev->retry_time += MESHX_TRANS_RETRY_PERIOD;
        if (pdev->retry_time > MESHX_TRANS_LOSS_TIME)
        {
            /* stop pb adv timer */
            if (NULL != pdev->pb_adv_timer)
            {
                meshx_timer_stop(pdev->pb_adv_timer);
            }
            MESHX_WARN("provision failed: receive no ack of state(%d)", pdev->dev.state);
            meshx_pb_adv_link_close(&pdev->dev, MESHX_LINK_CLOSE_REASON_TIMEOUT);
        }
    }
    else
    {
        MESHX_ERROR("invalid provision state: %d", pdev->dev.state);
    }
}

static void meshx_pb_adv_timeout_handler(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_PB_ADV;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

static void meshx_pb_adv_handle_timeout(void *pargs)
{
    meshx_pb_adv_dev_t *pdev = pargs;
    if (!meshx_pb_adv_is_device_valid(pdev))
    {
        MESHX_WARN("handle pb adv timeout failed: device may be deleted!");
        return;
    }

    switch (pdev->timer_state)
    {
    case MESHX_PB_ADV_TIMER_STATE_DELAY:
        meshx_pb_adv_handle_delay(pdev);
        break;
    case MESHX_PB_ADV_TIMER_STATE_RETRY:
        meshx_pb_adv_handle_retry(pdev);
        break;
    case MESHX_PB_ADV_TIMER_STATE_LINK_MONITOR:
        meshx_pb_adv_handle_link_loss(pdev);
        break;
    default:
        MESHX_ERROR("invalid timer state: %d", pdev->timer_state);
        break;
    }
}

void meshx_pb_adv_async_handle_timeout(meshx_async_msg_t msg)
{
    meshx_pb_adv_handle_timeout(msg.pdata);
}

static meshx_pb_adv_dev_t *meshx_find_prov_dev_by_link_id(uint32_t link_id)
{
    meshx_list_t *pnode = NULL;
    meshx_pb_adv_dev_t *pdev = NULL;
    meshx_list_foreach(pnode, &pb_adv_devs)
    {
        pdev = MESHX_CONTAINER_OF(pnode, meshx_pb_adv_dev_t, node);
        if (pdev->link_id == link_id)
        {
            break;
        }
    }

    return pdev;
}

#if MESHX_SUPPORT_ROLE_PROVISIONER
int32_t meshx_pb_adv_link_open(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);

    /* check whether is myself */
    if (0 == memcmp(meshx_node_params.config.dev_uuid, prov_dev->dev_uuid, sizeof(meshx_dev_uuid_t)))
    {
        MESHX_ERROR("can't not provision myself");
        return -MESHX_ERR_INVAL;
    }

    /* check device state */
    if (prov_dev->state > MESHX_PROV_STATE_IDLE)
    {
        MESHX_WARN("device is already in provision procedure: %d", prov_dev->state);
        return -MESHX_ERR_BUSY;
    }

    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;

    pdev->link_id = MESHX_ABS(meshx_rand());
    pdev->tx_trans_num = 0xff;
    pdev->acked_trans_num = 0x00;
    pdev->dev.state = MESHX_PROV_STATE_LINK_OPENING;

    /* start link open */
    pb_adv_link_open(prov_dev->bearer, pdev->link_id, prov_dev->dev_uuid);

    /* start timer for retry */
    pdev->retry_time = 0;
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_RETRY;
    meshx_timer_start(pdev->pb_adv_timer, MESHX_LINK_OPEN_RETRY_PERIOD);

    return MESHX_SUCCESS;
}
#endif

#if MESHX_SUPPORT_ROLE_DEVICE
int32_t meshx_pb_adv_link_ack(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    if (pdev->dev.state != MESHX_PROV_STATE_LINK_OPENING)
    {
        MESHX_ERROR("invalid provision state: %d", pdev->dev.state);
        return -MESHX_ERR_STATE;
    }
    int32_t ret = pb_adv_link_ack(prov_dev->bearer, pdev->link_id);
    if (MESHX_SUCCESS == ret)
    {
        MESHX_INFO("link opened: %d", pdev->link_id);
        pdev->dev.state = MESHX_PROV_STATE_LINK_OPENED;
        /* notify app link opened */
        meshx_prov_link_open_result_t result = MESHX_PROV_LINK_OPEN_SUCCESS;
        meshx_notify_prov_t notify_prov;
        notify_prov.metadata.prov_dev = &pdev->dev;
        notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_LINK_OPEN;
        notify_prov.pdata = &result;
        meshx_prov_handle_notify(pdev->dev.bearer, &notify_prov,
                                 sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_prov_link_open_result_t));

        /* start timer for link monitor */
        pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_LINK_MONITOR;
        meshx_timer_start(pdev->pb_adv_timer, MESHX_LINK_LOSS_TIME);
    }

    return ret;
}
#endif

int32_t meshx_pb_adv_link_close(meshx_prov_dev_t prov_dev, uint8_t reason)
{
    MESHX_ASSERT(NULL != prov_dev);

    /* check device state */
    if (prov_dev->state <= MESHX_PROV_STATE_LINK_OPENING)
    {
        MESHX_WARN("device is not in provision procedure: %d", prov_dev->state);
        return -MESHX_ERR_BUSY;
    }

    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    pdev->dev.state = MESHX_PROV_STATE_LINK_CLOSING;
    pdev->link_close_reason = reason;

    /* start link close */
    if (reason == MESHX_PROV_LINK_CLOSE_LINK_LOSS)
    {
        reason = MESHX_PROV_LINK_CLOSE_TIMEOUT;
    }
    pb_adv_link_close(prov_dev->bearer, pdev->link_id, reason);

    /* start timer for retry */
    pdev->retry_time = 0;
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_RETRY;
    meshx_timer_start(pdev->pb_adv_timer, MESHX_LINK_CLOSE_RETRY_PERIOD);

    return MESHX_SUCCESS;
}

#if MESHX_SUPPORT_ROLE_PROVISIONER
static uint8_t meshx_prov_require_trans_num(meshx_pb_adv_dev_t *pdev)
{
    pdev->tx_trans_num ++;
    if (pdev->tx_trans_num > 0x7f)
    {
        pdev->tx_trans_num = 0;
    }

    return pdev->tx_trans_num;
}
#endif

#if MESHX_SUPPORT_ROLE_DEVICE
static uint8_t meshx_dev_require_trans_num(meshx_pb_adv_dev_t *pdev)
{
    pdev->tx_trans_num ++;
    if (0 == pdev->tx_trans_num)
    {
        pdev->tx_trans_num = 0x80;
    }

    return pdev->tx_trans_num;
}
#endif

int32_t meshx_pb_adv_trans_ack(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_TRANS_ACK;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}

#if MESHX_SUPPORT_ROLE_PROVISIONER
int32_t meshx_pb_adv_invite(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    meshx_prov_require_trans_num(pdev);

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}
#endif

#if MESHX_SUPPORT_ROLE_DEVICE
int32_t meshx_pb_adv_capabilites(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    meshx_dev_require_trans_num(pdev);

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}
#endif

#if MESHX_SUPPORT_ROLE_PROVISIONER
int32_t meshx_pb_adv_start(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    meshx_dev_require_trans_num(pdev);

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}
#endif

int32_t meshx_pb_adv_public_key(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    if (MESHX_ROLE_DEVICE == prov_dev->role)
    {
        meshx_dev_require_trans_num(pdev);
    }
    else
    {
        meshx_prov_require_trans_num(pdev);
    }

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_input_complete(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    meshx_dev_require_trans_num(pdev);

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_confirmation(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    if (MESHX_ROLE_DEVICE == prov_dev->role)
    {
        meshx_dev_require_trans_num(pdev);
    }
    else
    {
        meshx_prov_require_trans_num(pdev);
    }

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_random(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    if (MESHX_ROLE_DEVICE == prov_dev->role)
    {
        meshx_dev_require_trans_num(pdev);
    }
    else
    {
        meshx_prov_require_trans_num(pdev);
    }

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_data(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    meshx_dev_require_trans_num(pdev);

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_complete(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    meshx_dev_require_trans_num(pdev);

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_failed(meshx_prov_dev_t prov_dev)
{
    MESHX_ASSERT(NULL != prov_dev);
    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    meshx_dev_require_trans_num(pdev);

    /* start timer for delay */
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_DELAY;
    pdev->timer_delay_type = MESHX_PB_ADV_TIMER_DELAY_TYPE_PROV_PDU;
    meshx_timer_start(pdev->pb_adv_timer, meshx_pb_adv_rand());

    return MESHX_SUCCESS;
}

static int32_t meshx_pb_adv_recv_link_open(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    if (len < MESHX_LINK_OPEN_PDU_LEN)
    {
        MESHX_WARN("invalid link open pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }
    const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
    /* check uuid */
    if (0 != memcmp(meshx_node_params.config.dev_uuid, ppkt->bearer_ctl.link_open.dev_uuid,
                    sizeof(meshx_dev_uuid_t)))
    {
        MESHX_INFO("receive dismatched uuid");
        return -MESHX_ERR_DIFF;
    }

    meshx_prov_dev_t prov_dev = meshx_prov_create_device(bearer,
                                                         meshx_node_params.config.dev_uuid, MESHX_ROLE_DEVICE);
    if (NULL == prov_dev)
    {
        MESHX_ERROR("can't handle link open message now");
        return -MESHX_ERR_RESOURCE;
    }

    meshx_pb_adv_dev_t *pdev = (meshx_pb_adv_dev_t *)prov_dev;
    /* check state and link id */
    if ((MESHX_PROV_STATE_LINK_OPENED == prov_dev->state) &&
        (pdev->link_id != link_id))
    {
        MESHX_INFO("receive dismatched link id: %d-%d", pdev->link_id, link_id);
        return -MESHX_ERR_DIFF;
    }

    if (MESHX_PROV_STATE_LINK_OPENED == prov_dev->state)
    {
        MESHX_INFO("receive link open message again");
        pb_adv_link_ack(bearer, pdev->link_id);
        return MESHX_SUCCESS;
    }

    /* check state */
    if (prov_dev->state > MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("device is already in provisioning procedure: %d", prov_dev->state);
        return -MESHX_ERR_ALREADY;
    }

    pdev->link_id = link_id;
    pdev->dev.state = MESHX_PROV_STATE_LINK_OPENING;
    pdev->tx_trans_num = 0x7f;
    pdev->acked_trans_num = 0xff;
    pdev->trans_data_len = 0;
    MESHX_INFO("receive link open: %d", link_id);

    return meshx_pb_adv_link_ack(prov_dev);
}

static int32_t meshx_pb_adv_recv_link_ack(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    if (len < MESHX_LINK_ACK_PDU_LEN)
    {
        MESHX_WARN("invalid link open pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }
    const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;

    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
    meshx_pb_adv_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_RESOURCE;
    }

    /* check state */
    if (pdev->dev.state > MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid device state: %d", pdev->dev.state);
        return -MESHX_ERR_STATE;
    }

    MESHX_INFO("link established: %d", link_id);

    /* start timer for link monitor */
    pdev->retry_time = 0;
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_LINK_MONITOR;
    meshx_timer_start(pdev->pb_adv_timer, MESHX_LINK_LOSS_TIME);

    pdev->dev.state = MESHX_PROV_STATE_LINK_OPENED;

    /* notify app link opened */
    meshx_prov_link_open_result_t result = MESHX_PROV_LINK_OPEN_SUCCESS;
    meshx_notify_prov_t notify_prov;
    notify_prov.metadata.prov_dev = &pdev->dev;
    notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_LINK_OPEN;
    notify_prov.pdata = &result;
    meshx_prov_handle_notify(pdev->dev.bearer, &notify_prov,
                             sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_prov_link_open_result_t));

    return MESHX_SUCCESS;
}

static int32_t meshx_pb_adv_recv_link_close(meshx_bearer_t bearer, const uint8_t *pdata,
                                            uint8_t len)
{
    if (len < MESHX_LINK_CLOSE_PDU_LEN)
    {
        MESHX_WARN("invalid link open pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }

    const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
    meshx_pb_adv_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_RESOURCE;
    }

    MESHX_INFO("link closed: %d", link_id);
    /* stop timer */
    if (NULL != pdev->pb_adv_timer)
    {
        meshx_timer_stop(pdev->pb_adv_timer);
    }

    /* notify app link closed */
    uint8_t reason = ppkt->bearer_ctl.link_close.reason;
    meshx_notify_prov_t notify_prov;
    notify_prov.metadata.prov_dev = &pdev->dev;
    notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_LINK_CLOSE;
    notify_prov.pdata = &reason;
    meshx_prov_handle_notify(pdev->dev.bearer, &notify_prov,
                             sizeof(meshx_notify_prov_metadata_t) + sizeof(uint8_t));

    return MESHX_SUCCESS;
}

static int32_t meshx_pb_adv_recv_bearer_ctl(meshx_bearer_t bearer, const uint8_t *pdata,
                                            uint8_t len)
{
    if (len < MESHX_BEARER_CTL_METADATA_LEN)
    {
        MESHX_WARN("invalid bearer ctl pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }

    const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;
    if (ppkt->metadata.trans_num != MESHX_BEARER_CTL_TRANS_NUM)
    {
        MESHX_WARN("the transition number of bearer control must be 0: %d", ppkt->metadata.trans_num);
        return -MESHX_ERR_INVAL;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (ppkt->bearer_ctl.metadata.bearer_opcode)
    {
#if MESHX_SUPPORT_ROLE_DEVICE
    case MESHX_BEARER_LINK_OPEN:
        ret = meshx_pb_adv_recv_link_open(bearer, pdata, len);
        break;
#endif
#if MESHX_SUPPORT_ROLE_PROVISIONER
    case MESHX_BEARER_LINK_ACK:
        ret = meshx_pb_adv_recv_link_ack(bearer, pdata, len);
        break;
#endif
    case MESHX_BEARER_LINK_CLOSE:
        ret = meshx_pb_adv_recv_link_close(bearer, pdata, len);
        break;
    default:
        MESHX_WARN("invalid bearer opcode: %d", ppkt->bearer_ctl.metadata.bearer_opcode);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

static int32_t meshx_pb_adv_recv_prov_pdu(meshx_pb_adv_dev_t *pdev)
{
    if (MESHX_PROV_STATE_FAILED ==  pdev->dev.state)
    {
        MESHX_WARN("alreay in provision failed procedure, do not process provision data!");
        return -MESHX_ERR_STATE;
    }

    MESHX_DEBUG("receive provision data: ");
    MESHX_DUMP_DEBUG(pdev->prov_rx_pdu.data, pdev->trans_data_len);

    uint8_t fcs = meshx_3gpp_crc(pdev->prov_rx_pdu.data, pdev->trans_data_len);
    if (fcs != pdev->fcs)
    {
        MESHX_ERROR("calculated fcs(%d) doesn't match received fcs(%d)", fcs, pdev->fcs);
        /* clear trans data to enable receive new trans packet */
        pdev->trans_data_len = 0;
        return -MESHX_ERR_FAIL;
    }

    meshx_pb_adv_trans_ack(&pdev->dev);

    return MESHX_SUCCESS;
}

bool meshx_pb_adv_is_recv_all_trans_seg(const meshx_pb_adv_dev_t *pdev)
{
    for (uint8_t i = 0; i < MESHX_TRANS_SEG_NUM_MAX  / 8; ++i)
    {
        if (0 != pdev->recv_seg[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}

static int32_t meshx_pb_adv_recv_trans_start(meshx_bearer_t bearer, const uint8_t *pdata,
                                             uint8_t len)
{
    if (len < MESHX_TRANS_START_METADATA_LEN)
    {
        MESHX_WARN("invalid trans start pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }

    const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
    meshx_pb_adv_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    if (pdev->dev.state < MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid provision state: %d", pdev->dev.state);
        return -MESHX_ERR_STATE;
    }

    if (pdev->acked_trans_num == ppkt->metadata.trans_num)
    {
        MESHX_INFO("message(%d) has already been acked", ppkt->metadata.trans_num);
        /* ignore data and acked again */
        pb_adv_trans_ack(pdev->dev.bearer, pdev->link_id, pdev->acked_trans_num);
        return MESHX_SUCCESS;
    }

    uint16_t total_len = MESHX_BE16_TO_HOST(ppkt->trans_start.metadata.total_len);
    if ((0 == total_len) || (total_len > MESHX_TRANS_DATA_VALID_MAX_LEN))
    {
        MESHX_WARN("invalid total length: %d", total_len);
        return -MESHX_ERR_LENGTH;
    }

    uint8_t calc_seg_num = meshx_calc_seg_num(total_len);
    if ((calc_seg_num - 1) != ppkt->trans_start.metadata.last_seg_num)
    {
        MESHX_WARN("receive trans start length(%d) does not match segment num(%d)", total_len,
                   ppkt->trans_start.metadata.last_seg_num + 1);
        return -MESHX_ERR_LENGTH;
    }

    if (0 == pdev->trans_data_len)
    {
        /* receive trans start first time */
        for (uint8_t i = 0; i < MESHX_TRANS_SEG_NUM_MAX / 8; ++i)
        {
            pdev->recv_seg[i] = 0;
        }
        for (uint8_t i = 0; i <= ppkt->trans_start.metadata.last_seg_num; ++i)
        {
            MESHX_BIT_FIELD_SET(pdev->recv_seg, i);
        }
    }

    if (!MESHX_IS_BIT_FIELD_SET(pdev->recv_seg, 0))
    {
        /* receive another trans start */
        MESHX_WARN("trans start has already been successfully received!");
        return -MESHX_ERR_ALREADY;
    }

    pdev->trans_data_len = total_len;
    pdev->rx_trans_num = ppkt->metadata.trans_num;
    pdev->last_seg_num = ppkt->trans_start.metadata.last_seg_num;
    pdev->fcs = ppkt->trans_start.metadata.fcs;

    uint8_t recv_data_len = len - MESHX_TRANS_START_METADATA_LEN;
    if (0 == ppkt->trans_start.metadata.last_seg_num)
    {
        if (recv_data_len != pdev->trans_data_len)
        {
            MESHX_WARN("segment(0) received length(%d) does not match required length(%d)", recv_data_len,
                       pdev->trans_data_len);
            return -MESHX_ERR_LENGTH;
        }
    }
    else
    {
        if (recv_data_len != MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN)
        {
            MESHX_WARN("segment(0) received length(%d) does not match required length(%d)", recv_data_len,
                       pdev->trans_data_len);
            return -MESHX_ERR_LENGTH;
        }
    }

    MESHX_BIT_FIELD_CLEAR(pdev->recv_seg, 0);
    memcpy(pdev->prov_rx_pdu.data, ppkt->trans_start.pdu, recv_data_len);

    int32_t ret = MESHX_SUCCESS;
    if (meshx_pb_adv_is_recv_all_trans_seg(pdev))
    {
        /* process provision pdu */
        meshx_pb_adv_recv_prov_pdu(pdev);
    }

    return ret;
}

static int32_t meshx_pb_adv_recv_trans_continue(meshx_bearer_t bearer, const uint8_t *pdata,
                                                uint8_t len)
{
    if (len < MESHX_TRANS_CONTINUE_METADATA_LEN)
    {
        MESHX_WARN("invalid trans continue pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }
    const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
    meshx_pb_adv_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    if (pdev->dev.state < MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid provision state: %d", pdev->dev.state);
        return -MESHX_ERR_STATE;
    }

    if (pdev->acked_trans_num == ppkt->metadata.trans_num)
    {
        MESHX_INFO("message(%d) has already been acked", ppkt->metadata.trans_num);
        /* ignore data and acked again */
        pb_adv_trans_ack(pdev->dev.bearer, pdev->link_id, pdev->acked_trans_num);
        return MESHX_SUCCESS;
    }

    if (pdev->rx_trans_num != ppkt->metadata.trans_num)
    {
        MESHX_WARN("receive dismatched trans num:%d-%d", pdev->rx_trans_num,
                   ppkt->metadata.trans_num);
        return -MESHX_ERR_DIFF;
    }

    if (0 == pdev->trans_data_len)
    {
        /* receive trans continue message before trans start message */
        MESHX_WARN("receive trans continue message before trans start message!");
        return -MESHX_ERR_TIMING;
    }

    if (ppkt->trans_continue.metadata.seg_index > pdev->last_seg_num)
    {
        MESHX_WARN("invalid segment index: %d", ppkt->trans_continue.metadata.seg_index);
        return -MESHX_ERR_INVAL;
    }

    if (!MESHX_IS_BIT_FIELD_SET(pdev->recv_seg,
                                ppkt->trans_continue.metadata.seg_index))
    {
        MESHX_WARN("segment %d has already been received!", ppkt->trans_continue.metadata.seg_index);
        return -MESHX_ERR_ALREADY;
    }

    uint8_t recv_data_len = len - MESHX_TRANS_CONTINUE_METADATA_LEN;
    if (ppkt->trans_continue.metadata.seg_index < pdev->last_seg_num)
    {
        if (recv_data_len != MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN)
        {
            MESHX_WARN("segment(%d) received length(%d) does not match required length(%d)",
                       ppkt->trans_continue.metadata.seg_index, recv_data_len, MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN);
            return -MESHX_ERR_LENGTH;
        }
    }
    else
    {
        uint8_t required_len = pdev->trans_data_len - MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN
                               - MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN * (pdev->last_seg_num - 1);
        if (recv_data_len != required_len)
        {
            MESHX_WARN("receive segment(%d) length(%d) does not match required length(%d)",
                       ppkt->trans_continue.metadata.seg_index, recv_data_len, required_len);
            return -MESHX_ERR_LENGTH;
        }
    }

    MESHX_BIT_FIELD_CLEAR(pdev->recv_seg, ppkt->trans_continue.metadata.seg_index);

    uint16_t offset = MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN + MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN *
                      (ppkt->trans_continue.metadata.seg_index - 1);
    memcpy(pdev->prov_rx_pdu.data + offset, ppkt->trans_continue.pdu, recv_data_len);

    int32_t ret = MESHX_SUCCESS;
    if (meshx_pb_adv_is_recv_all_trans_seg(pdev))
    {
        /* process provision pdu */
        meshx_pb_adv_recv_prov_pdu(pdev);
    }
    return ret;
}

static int32_t meshx_pb_adv_recv_trans_ack(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    if (len < sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_ack_metadata_t))
    {
        MESHX_WARN("invalid trans continue pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }
    const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
    meshx_pb_adv_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    if (pdev->tx_trans_num != ppkt->metadata.trans_num)
    {
        MESHX_WARN("receive dismatch trans ack number: %d-%d", pdev->tx_trans_num,
                   ppkt->metadata.trans_num);
        return -MESHX_ERR_DIFF;
    }

    MESHX_INFO("link(%d) receive trans ack(%d)", link_id, ppkt->metadata.trans_num);

    /* start timer for link monitor */
    pdev->retry_time = 0;
    pdev->timer_state = MESHX_PB_ADV_TIMER_STATE_LINK_MONITOR;
    meshx_timer_start(pdev->pb_adv_timer, MESHX_LINK_LOSS_TIME);

    /* notify app receive trans ack */
    meshx_notify_prov_t notify_prov;
    notify_prov.metadata.prov_dev = &pdev->dev;
    notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_TRANS_ACK;
    notify_prov.pdata = &pdev->dev.state;
    meshx_prov_handle_notify(pdev->dev.bearer, &notify_prov,
                             sizeof(meshx_notify_prov_metadata_t));

    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    MESHX_ASSERT(MESHX_BEARER_TYPE_ADV == bearer->type);
    int32_t ret = MESHX_SUCCESS;
    const meshx_pb_adv_pkt_t *ppb_adv_pkt = (const meshx_pb_adv_pkt_t *)pdata;
    switch (ppb_adv_pkt->pdu.gpcf)
    {
    case MESHX_GPCF_TRANS_START:
        ret = meshx_pb_adv_recv_trans_start(bearer, pdata, len);
        break;
    case MESHX_GPCF_TRANS_ACK:
        ret = meshx_pb_adv_recv_trans_ack(bearer, pdata, len);
        break;
    case MESHX_GPCF_TRANS_CONTINUE:
        ret = meshx_pb_adv_recv_trans_continue(bearer, pdata, len);
        break;
    case MESHX_GPCF_BEARER_CTL:
        ret = meshx_pb_adv_recv_bearer_ctl(bearer, pdata, len);
        break;
    default:
        MESHX_WARN("invalid gpcf: %d", ppb_adv_pkt->pdu.gpcf);
        break;
    }
    return ret;
}

meshx_prov_dev_t meshx_pb_adv_create_device(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid)
{
    meshx_prov_dev_t prov_dev = meshx_pb_adv_find_device(bearer, dev_uuid);
    if (NULL != prov_dev)
    {
        MESHX_INFO("provision device with same parameter already exists!");
        return prov_dev;
    }

    /* create new provision device */
    meshx_pb_adv_dev_t *pdev = meshx_malloc(sizeof(meshx_pb_adv_dev_t));
    if (NULL == pdev)
    {
        MESHX_ERROR("create pb adv device failed: out of memory!");
        return NULL;
    }
    memset(pdev, 0, sizeof(meshx_pb_adv_dev_t));

    /* create pb adv timer */
    int32_t ret = meshx_timer_create(&pdev->pb_adv_timer, MESHX_TIMER_MODE_REPEATED,
                                     meshx_pb_adv_timeout_handler, pdev);
    if (MESHX_SUCCESS != ret)
    {
        MESHX_ERROR("create pb adv device failed: create pb adv timer failed!");
        meshx_free(pdev);
        return NULL;
    }

    meshx_list_append(&pb_adv_devs, &pdev->node);

    MESHX_INFO("create pb adv device(0x%08x) success", pdev);

    return &pdev->dev;
}

