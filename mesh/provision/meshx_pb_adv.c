/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx_config.h"
#define TRACE_MODULE "MESHX_PB_ADV"
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
#include "meshx_provision_internal.h"


/* maximum trans segment number */
#define MESHX_TRANS_SEG_NUM_MAX                 0x40

#define MESHX_LINK_LOSS_TIME                    60000 /* unit is ms */
#define MESHX_LINK_RETRY_PERIOD                 200 /* unit is ms */

#define MESHX_TRANS_LOSS_TIME                   30000 /* unit is ms */
#define MESHX_TRANS_RETRY_PERIOD                500 /* unit is ms */

#define MESHX_BEARER_CTL_TRANS_NUM              0

#define MESHX_PROV_STATE_IDLE                   0
#define MESHX_PROV_STATE_LINK_OPENING           0x01
#define MESHX_PROV_STATE_LINK_OPENED            0x02
#define MESHX_PROV_STATE_INVITE                 0x03

#define MESHX_BEARER_CTL_METADATA_LEN           (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t))
#define MESHX_LINK_OPEN_PDU_LEN                 (MESHX_BEARER_CTL_METADATA_LEN + sizeof(meshx_pb_adv_link_open_t))
#define MESHX_LINK_ACK_PDU_LEN                  (MESHX_BEARER_CTL_METADATA_LEN + sizeof(meshx_pb_adv_link_ack_t))
#define MESHX_LINK_CLOSE_PDU_LEN                (MESHX_BEARER_CTL_METADATA_LEN + sizeof(meshx_pb_adv_link_close_t))
#define MESHX_TRANS_ACK_PDU_LEN                 (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_ack_metadata_t))

#define MESHX_TRANS_START_METADATA_LEN          (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_start_metadata_t))
#define MESHX_TRANS_START_DATA_MAX_LEN          (MESHX_PB_ADV_PDU_MAX_LEN - MESHX_TRANS_START_METADATA_LEN)
#define MESHX_TRANS_CONTINUE_METADATA_LEN       (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_continue_metadata_t))
#define MESHX_TRANS_CONTINUE_DATA_MAX_LEN       (MESHX_PB_ADV_PDU_MAX_LEN - MESHX_TRANS_CONTINUE_METADATA_LEN)

#define MESHX_TRANS_DATA_MAX_LEN                (MESHX_TRANS_START_DATA_MAX_LEN + MESHX_TRANS_SEG_NUM_MAX * MESHX_TRANS_CONTINUE_DATA_MAX_LEN)

#define MESHX_TRANS_DATA_VALID_MAX_LEN          64

typedef struct
{
    meshx_dev_uuid_t uuid;
    meshx_bearer_t bearer;

    uint16_t state;
    uint32_t link_id;

    uint8_t tx_trans_num;
    uint8_t rx_trans_num;
    uint8_t acked_trans_num;

    uint32_t retry_time;
    meshx_timer_t link_timer;
    meshx_timer_t trans_timer;

    union
    {
        meshx_provision_invite_t invite;
    } prov_tx_pdu;

    uint8_t last_seg_num;
    uint8_t recv_seg[MESHX_TRANS_SEG_NUM_MAX / 8];
    uint16_t trans_data_len;
    union
    {
        meshx_provision_pdu_t pdu;
        uint8_t data[MESHX_TRANS_DATA_VALID_MAX_LEN];
    } prov_rx_pdu;

    meshx_list_t node;
} meshx_prov_dev_t;

static meshx_list_t prov_devs;

int32_t meshx_pb_adv_init(void)
{
    meshx_list_init_head(&prov_devs);
    return MESHX_SUCCESS;
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

static int32_t meshx_pb_adv_send(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
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
        ret = meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                                segment_len);
        if (MESHX_SUCCESS != ret)
        {
            break;
        }
    }

    return ret;
}

#if MESHX_ROLE_PROVISIONER
static int32_t pb_adv_link_open(meshx_bearer_t bearer, uint32_t link_id, meshx_dev_uuid_t dev_uuid)
{
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_OPEN;
    memcpy(pb_adv_pkt.bearer_ctl.link_open.dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t));

    MESHX_INFO("link opening: %d", link_id);
    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV,
                             (const uint8_t *)&pb_adv_pkt, MESHX_LINK_OPEN_PDU_LEN);
}
#endif

#if MESHX_ROLE_DEVICE
static int32_t pb_adv_link_ack(meshx_bearer_t bearer, uint32_t link_id)
{
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_ACK;

    MESHX_INFO("link ack: %d", link_id);

    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV,
                             (const uint8_t *)&pb_adv_pkt, MESHX_LINK_ACK_PDU_LEN);
}
#endif

static int32_t pb_adv_link_close(meshx_bearer_t bearer, uint32_t link_id, uint8_t reason)
{
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_CLOSE;
    pb_adv_pkt.bearer_ctl.link_close.reason = reason;

    MESHX_INFO("link close: %d", link_id);
    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                             MESHX_LINK_CLOSE_PDU_LEN);
}

static int32_t pb_adv_trans_ack(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num)
{
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.trans_ack.metadata.gpcf = MESHX_GPCF_TRANS_ACK;
    pb_adv_pkt.trans_ack.metadata.padding = 0;
    MESHX_INFO("trans ack: %d", trans_num);
    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                             MESHX_TRANS_ACK_PDU_LEN);
}

#if MESHX_ROLE_PROVISIONER
static int32_t pb_adv_invite(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                             meshx_provision_invite_t invite)
{
    MESHX_INFO("invite: %d", invite.attention_duration);
    meshx_provision_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROVISION_TYPE_INVITE;
    prov_pdu.metadata.padding = 0;
    prov_pdu.invite = invite;
    return meshx_pb_adv_send(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_invite_t));
}
#endif

static void meshx_pb_adv_remove_dev(meshx_prov_dev_t *pdev)
{
    meshx_list_remove(&pdev->node);
    if (NULL != pdev->link_timer)
    {
        meshx_timer_delete(pdev->link_timer);
    }

    if (NULL != pdev->trans_timer)
    {
        meshx_timer_delete(pdev->trans_timer);
    }
    meshx_free(pdev);
}

static void meshx_link_timeout_handler(void *pargs)
{
    meshx_prov_dev_t *pprov_dev = pargs;
    switch (pprov_dev->state)
    {
    case MESHX_PROV_STATE_LINK_OPENING:
        pb_adv_link_open(pprov_dev->bearer, pprov_dev->link_id, pprov_dev->uuid);
        pprov_dev->retry_time += MESHX_LINK_RETRY_PERIOD;
        if (pprov_dev->retry_time > MESHX_LINK_LOSS_TIME)
        {
            MESHX_ERROR("provision failed: receive no link ack from device uuid!");
            MESHX_DUMP_ERROR(pprov_dev->uuid, sizeof(meshx_dev_uuid_t));
            meshx_pb_adv_remove_dev(pprov_dev);
        }
        break;
    default:
        MESHX_ERROR("link loss in state: %d", pprov_dev->state);
        pb_adv_link_close(pprov_dev->bearer, pprov_dev->link_id,
                          MESHX_LINK_CLOSE_REASON_TIMEOUT);
        meshx_pb_adv_remove_dev(pprov_dev);
        break;
    }
}

static void meshx_trans_timeout_handler(void *pargs)
{
    meshx_prov_dev_t *pprov_dev = pargs;
    switch (pprov_dev->state)
    {
    case MESHX_PROV_STATE_INVITE:
        pb_adv_invite(pprov_dev->bearer, pprov_dev->link_id,
                      pprov_dev->tx_trans_num, pprov_dev->prov_tx_pdu.invite);
        pprov_dev->retry_time += MESHX_TRANS_RETRY_PERIOD;
        if (pprov_dev->retry_time > MESHX_TRANS_LOSS_TIME)
        {
            MESHX_ERROR("provision failed: receive no ack of state(%d)", pprov_dev->state);
            pb_adv_link_close(pprov_dev->bearer, pprov_dev->link_id,
                              MESHX_LINK_CLOSE_REASON_TIMEOUT);
            meshx_pb_adv_remove_dev(pprov_dev);
        }
        break;
    default:
        break;
    }
}

static meshx_prov_dev_t *meshx_find_prov_dev_by_uuid(const meshx_dev_uuid_t dev_uuid)
{
    meshx_list_t *pnode;
    meshx_prov_dev_t *pprov_dev = NULL;
    meshx_list_foreach(pnode, &prov_devs)
    {
        pprov_dev = MESHX_CONTAINER_OF(pnode, meshx_prov_dev_t, node);
        if (0 == memcmp(pprov_dev->uuid, dev_uuid, sizeof(meshx_dev_uuid_t)))
        {
            break;
        }
    }

    return pprov_dev;
}

static meshx_prov_dev_t *meshx_find_prov_dev_by_link_id(uint32_t link_id)
{
    meshx_list_t *pnode;
    meshx_prov_dev_t *pprov_dev = NULL;
    meshx_list_foreach(pnode, &prov_devs)
    {
        pprov_dev = MESHX_CONTAINER_OF(pnode, meshx_prov_dev_t, node);
        if (pprov_dev->link_id == link_id)
        {
            break;
        }
    }

    return pprov_dev;
}

#if MESHX_ROLE_PROVISIONER
int32_t meshx_pb_adv_link_open(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid)
{
    /* find exist first */
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_uuid(dev_uuid);
    if (NULL == pdev)
    {
        /* check whether is myself */
        meshx_dev_uuid_t uuid_self;
        meshx_get_device_uuid(uuid_self);
        if (0 == memcmp(uuid_self, dev_uuid, sizeof(meshx_dev_uuid_t)))
        {
            MESHX_ERROR("can't not provision myself");
            return -MESHX_ERR_INVAL;
        }

        /* create new provision device */
        pdev = meshx_malloc(sizeof(meshx_prov_dev_t));
        if (NULL == pdev)
        {
            MESHX_ERROR("link open failed: out of memory!");
            return -MESHX_ERR_NO_MEM;
        }
        memset(pdev, 0, sizeof(meshx_prov_dev_t));
    }
    else
    {
        /* existing provision deivice */
        if (pdev->state > MESHX_PROV_STATE_IDLE)
        {
            MESHX_WARN("device is already in provision procedure");
            return -MESHX_ERR_BUSY;
        }
    }

    pdev->bearer = bearer;
    pdev->link_id = MESHX_ABS(meshx_rand());
    memcpy(pdev->uuid, dev_uuid, sizeof(meshx_dev_uuid_t));

    /* create link and trans timer */
    int32_t ret = meshx_timer_create(&pdev->link_timer, MESHX_TIMER_MODE_REPEATED,
                                     meshx_link_timeout_handler, pdev);
    if (MESHX_SUCCESS != ret)
    {
        MESHX_ERROR("link open failed: create link timer failed!");
        meshx_free(pdev);
        return ret;
    }

    ret = meshx_timer_create(&pdev->trans_timer, MESHX_TIMER_MODE_REPEATED,
                             meshx_trans_timeout_handler, pdev);
    if (MESHX_SUCCESS != ret)
    {
        MESHX_ERROR("link open failed: create trans timer failed!");
        meshx_timer_delete(pdev->link_timer);
        meshx_free(pdev);
        return ret;
    }

    meshx_list_append(&prov_devs, &pdev->node);

    /* start link open */
    pb_adv_link_open(bearer, pdev->link_id, dev_uuid);
    uint8_t old_state = pdev->state;
    pdev->state = MESHX_PROV_STATE_LINK_OPENING;

    /* notify upper layer state changed */
    meshx_provision_state_changed(pdev->uuid, MESHX_PROV_STATE_LINK_OPENING, old_state);

    /* start retry timer */
    meshx_timer_start(pdev->link_timer, MESHX_LINK_RETRY_PERIOD);

    return MESHX_SUCCESS;
}
#endif

#if MESHX_ROLE_DEVICE
static int32_t meshx_pb_adv_link_ack_internal(meshx_prov_dev_t *pdev)
{
    MESHX_ASSERT(NULL != pdev);
    int32_t ret = pb_adv_link_ack(pdev->bearer, pdev->link_id);
    if (MESHX_SUCCESS == ret)
    {
        MESHX_INFO("link opened: %d", pdev->link_id);
        uint8_t old_state = pdev->state;
        pdev->state = MESHX_PROV_STATE_LINK_OPENED;

        /* notify upper layer state changed */
        meshx_provision_state_changed(pdev->uuid, MESHX_PROV_STATE_LINK_OPENED, old_state);

        /* start link loss timer */
        meshx_timer_start(pdev->link_timer, MESHX_LINK_LOSS_TIME);
    }

    return ret;
}

int32_t meshx_pb_adv_link_ack(meshx_dev_uuid_t dev_uuid)
{
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_uuid(dev_uuid);
    if (NULL == pdev)
    {
        MESHX_ERROR("invalid device uuid");
        return -MESHX_ERR_INVAL;
    }

    return meshx_pb_adv_link_ack_internal(pdev);
}
#endif

static int32_t meshx_pb_adv_link_close_internal(meshx_prov_dev_t *pdev, uint8_t reason)
{
    MESHX_ASSERT(NULL != pdev);
    int32_t ret = pb_adv_link_close(pdev->bearer, pdev->link_id, reason);
    if (MESHX_SUCCESS == ret)
    {
        /* notify upper layer state changed */
        meshx_provision_state_changed(pdev->uuid, MESHX_PROV_STATE_IDLE, pdev->state);

        /* remove device */
        meshx_pb_adv_remove_dev(pdev);
    }

    return ret;
}

int32_t meshx_pb_adv_link_close(meshx_dev_uuid_t dev_uuid, uint8_t reason)
{
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_uuid(dev_uuid);
    if (NULL == pdev)
    {
        MESHX_ERROR("invalid device uuid");
        return -MESHX_ERR_INVAL;
    }

    return meshx_pb_adv_link_close_internal(pdev, reason);
}

#if MESHX_ROLE_PROVISIONER
static uint8_t meshx_prov_require_trans_num(meshx_prov_dev_t *pdev)
{
    pdev->tx_trans_num ++;
    if (pdev->tx_trans_num > 0x7f)
    {
        pdev->tx_trans_num = 0;
    }

    return pdev->tx_trans_num;
}
#endif

#if MESHX_ROLE_DEVICE
#if 0
static uint8_t meshx_dev_require_trans_num(meshx_prov_dev_t *pdev)
{
    pdev->tx_trans_num ++;
    if (0 == pdev->tx_trans_num)
    {
        pdev->tx_trans_num = 0x80;
    }

    return pdev->tx_trans_num;
}
#endif
#endif

static int32_t meshx_pb_adv_trans_ack_internal(meshx_prov_dev_t *pdev)
{
    return pb_adv_trans_ack(pdev->bearer, pdev->link_id, pdev->rx_trans_num);
}

int32_t meshx_pb_adv_trans_ack(meshx_dev_uuid_t dev_uuid)
{
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_uuid(dev_uuid);
    if (NULL == pdev)
    {
        MESHX_ERROR("invalid device");
        return -MESHX_ERR_INVAL;
    }

    return meshx_pb_adv_trans_ack_internal(pdev);
}

static int32_t meshx_pb_adv_invite_internal(meshx_prov_dev_t *pdev, meshx_provision_invite_t invite)
{
    MESHX_ASSERT(NULL != pdev);
    if ((pdev->state < MESHX_PROV_STATE_LINK_OPENED) ||
        (pdev->state > MESHX_PROV_STATE_INVITE))
    {
        MESHX_ERROR("invalid state: %d", pdev->state);
        return -MESHX_ERR_STATE;
    }

    if (MESHX_PROV_STATE_INVITE == pdev->state)
    {
        MESHX_WARN("already in invite procedure");
        return -MESHX_ERR_ALREADY;
    }

    pb_adv_invite(pdev->bearer, pdev->link_id, meshx_prov_require_trans_num(pdev), invite);

    uint8_t old_state = pdev->state;
    pdev->state = MESHX_PROV_STATE_INVITE;
    pdev->prov_tx_pdu.invite = invite;

    /* notify upper layer state changed */
    meshx_provision_state_changed(pdev->uuid, MESHX_PROV_STATE_INVITE, old_state);

    /* start retry timer */
    meshx_timer_start(pdev->trans_timer, MESHX_TRANS_RETRY_PERIOD);

    return MESHX_SUCCESS;
}


int32_t meshx_pb_adv_invite(meshx_dev_uuid_t dev_uuid,
                            meshx_provision_invite_t invite)
{
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_uuid(dev_uuid);
    if (NULL == pdev)
    {
        MESHX_ERROR("invalid device");
        return -MESHX_ERR_INVAL;
    }

    return meshx_pb_adv_invite_internal(pdev, invite);
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
    meshx_dev_uuid_t dev_uuid;
    meshx_get_device_uuid(dev_uuid);
    if (0 != memcmp(dev_uuid, ppkt->bearer_ctl.link_open.dev_uuid, sizeof(meshx_dev_uuid_t)))
    {
        MESHX_INFO("receive dismatched uuid");
        return -MESHX_ERR_DIFF;
    }

    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_uuid(dev_uuid);
    if (NULL == pdev)
    {
        /* first receive link open message */
        pdev = meshx_malloc(sizeof(meshx_prov_dev_t));
        if (NULL == pdev)
        {
            MESHX_ERROR("can't process link open request: out of memory");
            return -MESHX_ERR_NO_MEM;
        }
        memset(pdev, 0, sizeof(meshx_prov_dev_t));

        /* create link and trans timer */
        int32_t ret = meshx_timer_create(&pdev->link_timer, MESHX_TIMER_MODE_REPEATED,
                                         meshx_link_timeout_handler, pdev);
        if (MESHX_SUCCESS != ret)
        {
            MESHX_ERROR("can't process link open request: create link timer failed!");
            meshx_free(pdev);
            return ret;
        }

        ret = meshx_timer_create(&pdev->trans_timer, MESHX_TIMER_MODE_REPEATED,
                                 meshx_trans_timeout_handler, pdev);
        if (MESHX_SUCCESS != ret)
        {
            MESHX_ERROR("can't process link open request: create trans timer failed!");
            meshx_timer_delete(pdev->link_timer);
            meshx_free(pdev);
            return ret;
        }
    }

    /* check state and link id */
    if ((MESHX_PROV_STATE_LINK_OPENED == pdev->state) &&
        (pdev->link_id != link_id))
    {
        MESHX_INFO("receive dismatched link id: %d-%d", pdev->link_id, link_id);
        return -MESHX_ERR_DIFF;
    }

    if (MESHX_PROV_STATE_LINK_OPENED == pdev->state)
    {
        MESHX_INFO("receive link open message again");
        pb_adv_link_ack(bearer, pdev->link_id);
        return MESHX_SUCCESS;
    }

    /* check state */
    if (pdev->state > MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("device is already in provisioning procedure: %d", pdev->state);
        return -MESHX_ERR_ALREADY;
    }

    MESHX_INFO("receive link open: %d", link_id);
    meshx_list_append(&prov_devs, &pdev->node);

    pdev->bearer = bearer;
    uint8_t old_state = pdev->state;
    pdev->state = MESHX_PROV_STATE_LINK_OPENING;

    /* notify upper layer state changed */
    meshx_provision_state_changed(pdev->uuid, MESHX_PROV_STATE_LINK_OPENING, old_state);

    return meshx_pb_adv_link_ack_internal(pdev);
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
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    /* check state */
    if (pdev->state > MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid device state: %d", pdev->state);
        return -MESHX_ERR_STATE;
    }

    MESHX_INFO("link established: %d", link_id);

    pdev->tx_trans_num = 0xff;
    pdev->acked_trans_num = 0x00;
    uint8_t old_state = pdev->state;
    pdev->state = MESHX_PROV_STATE_LINK_OPENED;

    /* notify upper layer state changed */
    meshx_provision_state_changed(pdev->uuid, MESHX_PROV_STATE_LINK_OPENED, old_state);

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
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    /* notify upper layer state changed */
    meshx_provision_state_changed(pdev->uuid, MESHX_PROV_STATE_IDLE, pdev->state);

    meshx_pb_adv_remove_dev(pdev);

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
#if MESHX_ROLE_DEVICE
    case MESHX_BEARER_LINK_OPEN:
        ret = meshx_pb_adv_recv_link_open(bearer, pdata, len);
        break;
#endif
#if MESHX_ROLE_PROVISIONER
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

static int32_t meshx_pb_adv_recv_prov_pdu(meshx_prov_dev_t *pdev)
{
    MESHX_DEBUG("receive provision data: ");
    MESHX_DUMP_DEBUG(pdev->prov_rx_pdu.data, pdev->trans_data_len);

    pdev->acked_trans_num = pdev->rx_trans_num;
    /* send trans ack */
    meshx_pb_adv_trans_ack_internal(pdev);

    return meshx_provision_pdu_process(pdev->bearer, pdev->uuid, pdev->prov_rx_pdu.data,
                                       pdev->trans_data_len);
}

bool meshx_pb_adv_is_recv_all_trans_seg(const meshx_prov_dev_t *pdev)
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
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    if (pdev->state < MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid provision state: %d", pdev->state);
        return -MESHX_ERR_STATE;
    }

    if (pdev->acked_trans_num == ppkt->metadata.trans_num)
    {
        /* TODO: ignore data and acked again */
        MESHX_INFO("message(%d) has already been acked", ppkt->metadata.trans_num);
        return MESHX_SUCCESS;
    }

    if (!MESHX_IS_BIT_FIELD_SET(pdev->recv_seg, 0))
    {
        /* receive another trans start */
        MESHX_WARN("trans start has already been successfully received!");
        return -MESHX_ERR_ALREADY;
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

    pdev->trans_data_len = total_len;
    pdev->rx_trans_num = ppkt->metadata.trans_num;
    for (uint8_t i = 0; i < MESHX_TRANS_SEG_NUM_MAX / 8; ++i)
    {
        pdev->recv_seg[i] = 0;
    }
    for (uint8_t i = 0; i <= ppkt->trans_start.metadata.last_seg_num; ++i)
    {
        MESHX_BIT_FIELD_SET(pdev->recv_seg, i);
    }
    pdev->last_seg_num = ppkt->trans_start.metadata.last_seg_num;

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
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    if (pdev->state < MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid provision state: %d", pdev->state);
        return -MESHX_ERR_STATE;
    }

    if (pdev->acked_trans_num == ppkt->metadata.trans_num)
    {
        /* TODO: ignore data and acked again */
        MESHX_INFO("message(%d) has already been acked", ppkt->metadata.trans_num);
        return MESHX_SUCCESS;
    }

    if (pdev->rx_trans_num != ppkt->metadata.trans_num)
    {
        MESHX_WARN("receive dismatched trans num:%d-%d", pdev->rx_trans_num,
                   ppkt->metadata.trans_num);
        return -MESHX_ERR_FAIL;
    }

    if (0 == pdev->trans_data_len)
    {
        /* receive trans continue message before trans start message */
        MESHX_WARN("receive trans continue message before trans start message!");
        return -MESHX_ERR_FAIL;
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
    if (len < sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_start_metadata_t))
    {
        MESHX_WARN("invalid trans continue pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }
    const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
    meshx_prov_dev_t *pdev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pdev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    int32_t ret = MESHX_SUCCESS;
    return ret;
}

int32_t meshx_pb_adv_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    MESHX_ASSERT(MESHX_BEARER_TYPE_ADV == bearer.type);
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
