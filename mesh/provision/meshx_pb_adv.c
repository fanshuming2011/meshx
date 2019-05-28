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

/* maximum trans segment number */
#define MESHX_PB_ADV_TRANS_SEG_NUM_MAX          0x3f

#define MESHX_LINK_LOSS_TIME                    60000 /* unit is ms */
#define MESHX_LINK_RETRY_PERIOD                 200 /* unit is ms */

#define MESHX_TRANS_LOSS_TIME                   3000 /* unit is ms */
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

#define MESHX_TRANS_START_METADATA_LEN          (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_start_metadata_t))
#define MESHX_TRANS_CONTINUE_METADATA_LEN       (sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_continue_metadata_t))

typedef struct
{
    meshx_bearer_t bearer;
    uint16_t state;
    uint32_t link_id;
    uint8_t tx_trans_num;
    uint8_t rx_trans_num;
    uint32_t retry_time;
    meshx_timer_t timer;
    union
    {
        meshx_provision_invite_t invite;
    } prov_tx_pdu;

    uint8_t acked_trans_num;
    struct
    {
        uint8_t last_seg_num;
        uint8_t recv_seg[(MESHX_PB_ADV_TRANS_SEG_NUM_MAX + 1) / 8];
        uint8_t *pdata;
        uint16_t total_data_len;
    } prov_rx_pdu;
} meshx_prov_info_t;

#if MESHX_ROLE_PROVISIONER
typedef struct
{
    meshx_prov_info_t prov_info;
    meshx_dev_uuid_t uuid;
    meshx_list_t node;
} meshx_prov_dev_t;

/* for provisioner use */
static meshx_list_t prov_devs;
#endif

#if MESHX_ROLE_DEVICE
/* for device use */
static meshx_prov_info_t prov_self;
#endif

int32_t meshx_pb_adv_init(void)
{
#if MESHX_ROLE_PROVISIONER
    meshx_list_init_head(&prov_devs);
#endif

#if MESHX_ROLE_DEVICE
    memset(&prov_self, 0, sizeof(meshx_prov_info_t));
#endif

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
    MESHX_ASSERT((segment_num - 1) <= MESHX_PB_ADV_TRANS_SEG_NUM_MAX);

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

#if MESHX_ROLE_PROVISIONER
static int32_t pb_adv_invite(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                             meshx_provision_invite_t invite)
{
#if 0
    MESHX_INFO("invite: %d", invite);
    meshx_provision_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROVISION_TYPE_INVITE;
    prov_pdu.invite = invite;
    return meshx_pb_adv_send(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_invite_t));
#endif
    MESHX_INFO("invite: %d", invite);
    meshx_provision_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROVISION_TYPE_INVITE;
    for (uint8_t i = 1; i < 66; ++i)
    {
        prov_pdu.data[i] = i;
    }
    return meshx_pb_adv_send(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_provision_pdu_metadata_t) + 65);

}
#endif

#if MESHX_ROLE_PROVISIONER
static void meshx_pb_adv_remove_dev(meshx_prov_dev_t *pdev)
{
    meshx_list_remove(&pdev->node);
    if (NULL != pdev->prov_info.timer)
    {
        meshx_timer_delete(pdev->prov_info.timer);
    }

    meshx_free(pdev);
}

static void meshx_pb_adv_devs_timeout_handler(void *pargs)
{
    meshx_prov_dev_t *pprov_dev = pargs;
    switch (pprov_dev->prov_info.state)
    {
    case MESHX_PROV_STATE_LINK_OPENING:
        pb_adv_link_open(pprov_dev->prov_info.bearer, pprov_dev->prov_info.link_id, pprov_dev->uuid);
        pprov_dev->prov_info.retry_time += MESHX_LINK_RETRY_PERIOD;
        if (pprov_dev->prov_info.retry_time > MESHX_LINK_LOSS_TIME)
        {
            MESHX_ERROR("provision failed: receive no link ack from device uuid!");
            MESHX_DUMP_ERROR(pprov_dev->uuid, sizeof(meshx_dev_uuid_t));
            meshx_pb_adv_remove_dev(pprov_dev);
        }
        break;
    case MESHX_PROV_STATE_INVITE:
        MESHX_INFO("invite: ");
        pb_adv_invite(pprov_dev->prov_info.bearer, pprov_dev->prov_info.link_id,
                      pprov_dev->prov_info.tx_trans_num, pprov_dev->prov_info.prov_tx_pdu.invite);
        pprov_dev->prov_info.retry_time += MESHX_TRANS_RETRY_PERIOD;
        if (pprov_dev->prov_info.retry_time > MESHX_TRANS_LOSS_TIME)
        {
            MESHX_ERROR("provision failed: receive no ack of state(%d)", pprov_dev->prov_info.state);
            meshx_pb_adv_link_close(pprov_dev->prov_info.bearer, pprov_dev->prov_info.link_id,
                                    MESHX_LINK_CLOSE_REASON_TIMEOUT);
            meshx_pb_adv_remove_dev(pprov_dev);
        }
        break;
    default:
        break;
    }
}
#endif

#if MESHX_ROLE_DEVICE
void meshx_pb_adv_self_timeout_handler(void *pargs)
{

}
#endif

#if MESHX_ROLE_PROVISIONER
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
        if (pprov_dev->prov_info.link_id == link_id)
        {
            break;
        }
    }

    return pprov_dev;
}

int32_t meshx_pb_adv_link_open(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid)
{
    /* find exist first */
    meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_uuid(dev_uuid);
    if (NULL == pprov_dev)
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
        pprov_dev = meshx_malloc(sizeof(meshx_prov_dev_t));
        if (NULL == pprov_dev)
        {
            MESHX_ERROR("link open failed: out of memory!");
            return -MESHX_ERR_NO_MEM;
        }
        memset(pprov_dev, 0, sizeof(meshx_prov_dev_t));
    }
    else
    {
        /* existing provision deivice */
        if (pprov_dev->prov_info.state > MESHX_PROV_STATE_IDLE)
        {
            MESHX_WARN("device is already in provision procedure");
            return -MESHX_ERR_BUSY;
        }
    }

    pprov_dev->prov_info.bearer = bearer;
    pprov_dev->prov_info.link_id = MESHX_ABS(meshx_rand());
    memcpy(pprov_dev->uuid, dev_uuid, sizeof(meshx_dev_uuid_t));

    int32_t ret = meshx_timer_create(&pprov_dev->prov_info.timer, MESHX_TIMER_MODE_REPEATED,
                                     meshx_pb_adv_devs_timeout_handler, pprov_dev);
    if (MESHX_SUCCESS != ret)
    {
        MESHX_ERROR("link open failed: create timer failed!");
        meshx_free(pprov_dev);
        return ret;
    }

    meshx_list_append(&prov_devs, &pprov_dev->node);

    pb_adv_link_open(bearer, pprov_dev->prov_info.link_id, dev_uuid);

    pprov_dev->prov_info.state = MESHX_PROV_STATE_LINK_OPENING;
    meshx_timer_start(pprov_dev->prov_info.timer, MESHX_LINK_RETRY_PERIOD);

    return MESHX_SUCCESS;
}
#endif

#if MESHX_ROLE_DEVICE
int32_t meshx_pb_adv_link_ack(meshx_bearer_t bearer, uint32_t link_id)
{
    int32_t ret = pb_adv_link_ack(bearer, link_id);
    if (MESHX_SUCCESS == ret)
    {
        if (MESHX_PROV_STATE_LINK_OPENED != prov_self.state)
        {
            prov_self.state = MESHX_PROV_STATE_LINK_OPENED;
            /* TODO: notify app state changed */
        }
    }

    return ret;
}
#endif

int32_t meshx_pb_adv_link_close(meshx_bearer_t bearer, uint32_t link_id, uint8_t reason)
{
    return pb_adv_link_close(bearer, link_id, reason);
}

#if MESHX_ROLE_PROVISIONER
static uint8_t meshx_prov_require_trans_num(meshx_prov_info_t *pinfo)
{
    pinfo->tx_trans_num ++;
    if (pinfo->tx_trans_num > 0x7f)
    {
        pinfo->tx_trans_num = 0;
    }

    return pinfo->tx_trans_num;
}
#endif

#if MESHX_ROLE_DEVICE
#if 0
static uint8_t meshx_dev_require_trans_num(meshx_prov_info_t *pinfo)
{
    pinfo->tx_trans_num ++;
    if (0 == pinfo->tx_trans_num)
    {
        pinfo->tx_trans_num = 0x80;
    }

    return pinfo->tx_trans_num;
}
#endif
#endif

int32_t meshx_pb_adv_invite(meshx_bearer_t bearer, uint32_t link_id,
                            meshx_provision_invite_t invite)
{
    meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pprov_dev)
    {
        MESHX_ERROR("invalid link id: %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    meshx_timer_start(pprov_dev->prov_info.timer, MESHX_TRANS_RETRY_PERIOD);
    pprov_dev->prov_info.state = MESHX_PROV_STATE_INVITE;
    /* todo: notify app state changed */
    pb_adv_invite(bearer, link_id, meshx_prov_require_trans_num(&pprov_dev->prov_info), invite);

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
    meshx_dev_uuid_t dev_uuid;
    meshx_get_device_uuid(dev_uuid);
    if (0 != memcmp(dev_uuid, ppkt->bearer_ctl.link_open.dev_uuid, sizeof(meshx_dev_uuid_t)))
    {
        MESHX_INFO("receive dismatched uuid");
        return -MESHX_ERR_DIFF;
    }

    /* check state and link id */
    if ((MESHX_PROV_STATE_LINK_OPENED == prov_self.state) &&
        (prov_self.link_id != link_id))
    {
        MESHX_INFO("receive dismatched link id: %d-%d", prov_self.link_id, link_id);
        return -MESHX_ERR_DIFF;
    }

    /* check state */
    if (prov_self.state > MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("device is already in provisioning procedure: %d-%d", prov_self.state,
                   prov_self.link_id);
        return -MESHX_ERR_BUSY;
    }

    MESHX_INFO("receive link open: %d", link_id);
    prov_self.link_id = link_id;
    prov_self.acked_trans_num = 0xff;

    /* TODO: start link idle timer */
    /* link ack */
    meshx_pb_adv_link_ack(bearer, prov_self.link_id);

    return MESHX_SUCCESS;
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
    meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(link_id);
    if (NULL == pprov_dev)
    {
        MESHX_WARN("can't find provision device with link id %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    /* check state */
    if (pprov_dev->prov_info.state > MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid device state: %d-%d", pprov_dev->prov_info.link_id,
                   pprov_dev->prov_info.state);
        return -MESHX_ERR_STATE;
    }

    MESHX_INFO("link established with id %d", link_id);

    pprov_dev->prov_info.state = MESHX_PROV_STATE_LINK_OPENED;
    pprov_dev->prov_info.tx_trans_num = 0xff;
    pprov_dev->prov_info.acked_trans_num = 0x00;
    /* TODO: notify app state changed */

    /* start invite */
    meshx_provision_invite_t invite = {0};
    /* TODO: get invite from app */
    int32_t ret = meshx_pb_adv_invite(bearer, link_id, invite);
    if (MESHX_SUCCESS == ret)
    {
        meshx_timer_change_interval(pprov_dev->prov_info.timer, MESHX_TRANS_RETRY_PERIOD);
    }
    else
    {
        /* send invite failed */
        meshx_pb_adv_link_close(bearer, link_id, MESHX_LINK_CLOSE_REASON_FAIL);
        meshx_pb_adv_remove_dev(pprov_dev);
        /* TODO: notify app state changed */
    }

    return ret;
}

static int32_t meshx_pb_adv_recv_link_close(meshx_bearer_t bearer, const uint8_t *pdata,
                                            uint8_t len)
{
    if (len < MESHX_LINK_CLOSE_PDU_LEN)
    {
        MESHX_WARN("invalid link open pdu length: %d", len);
        return -MESHX_ERR_LENGTH;
    }
    //const meshx_pb_adv_pkt_t *ppkt = (const meshx_pb_adv_pkt_t *)pdata;

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

static int32_t meshx_pb_adv_recv_prov_pdu(meshx_bearer_t bearer, const uint8_t *pdata,
                                          uint16_t len)
{
    MESHX_DEBUG("receive provision data: ");
    MESHX_DUMP_DEBUG(pdata, len);
    return MESHX_SUCCESS;
}

bool meshx_pb_adv_is_recv_all_trans_seg(const meshx_prov_info_t *pinfo)
{
    for (uint8_t i = 0; i < (MESHX_PB_ADV_TRANS_SEG_NUM_MAX + 1) / 8; ++i)
    {
        if (0 != pinfo->prov_rx_pdu.recv_seg[i])
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

    meshx_prov_info_t *pprov_info = NULL;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
#if MESHX_ROLE_DEVICE
    /* check link id first */
    if (link_id == prov_self.link_id)
    {
        pprov_info = &prov_self;
    }
#endif
    else
    {
#if MESHX_ROLE_PROVISIONER
        meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(link_id);
        if (NULL == pprov_dev)
        {
            pprov_info = &pprov_dev->prov_info;
        }
#endif
    }

    if (NULL == pprov_info)
    {
        MESHX_WARN("receive invalid link id: %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    if (pprov_info->state < MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid provision state: %d", pprov_info->state);
        return -MESHX_ERR_STATE;
    }

    if (pprov_info->acked_trans_num == ppkt->metadata.trans_num)
    {
        /* TODO: ignore data and acked again */
        MESHX_INFO("message(%d) has already been acked", ppkt->metadata.trans_num);
        return MESHX_SUCCESS;
    }

    if ((NULL != pprov_info->prov_rx_pdu.pdata) &&
        !MESHX_IS_BIT_FIELD_SET(pprov_info->prov_rx_pdu.recv_seg, 0))
    {
        /* receive another trans start */
        MESHX_WARN("trans start has already been successfully received!");
        return -MESHX_ERR_ALREADY;
    }

    uint16_t total_len = MESHX_BE16_TO_HOST(ppkt->trans_start.metadata.total_len);
    uint8_t calc_seg_num = meshx_calc_seg_num(total_len);
    if ((calc_seg_num - 1) != ppkt->trans_start.metadata.last_seg_num)
    {
        MESHX_WARN("receive trans start length(%d) does not match segment num(%d)", total_len,
                   ppkt->trans_start.metadata.last_seg_num + 1);
        return -MESHX_ERR_LENGTH;
    }

    if (NULL == pprov_info->prov_rx_pdu.pdata)
    {
        pprov_info->prov_rx_pdu.pdata = meshx_malloc(total_len);
        if (NULL == pprov_info->prov_rx_pdu.pdata)
        {
            MESHX_WARN("can not allocate memory for data reassembly");
            return -MESHX_ERR_NO_MEM;
        }
    }

    pprov_info->prov_rx_pdu.total_data_len = total_len;

    pprov_info->rx_trans_num = ppkt->metadata.trans_num;
    for (uint8_t i = 0; i < (MESHX_PB_ADV_TRANS_SEG_NUM_MAX + 1) / 8; ++i)
    {
        pprov_info->prov_rx_pdu.recv_seg[i] = 0;
    }
    for (uint8_t i = 0; i <= ppkt->trans_start.metadata.last_seg_num; ++i)
    {
        MESHX_BIT_FIELD_SET(pprov_info->prov_rx_pdu.recv_seg, i);
    }
    pprov_info->prov_rx_pdu.last_seg_num = ppkt->trans_start.metadata.last_seg_num;

    uint8_t recv_data_len = len - MESHX_TRANS_START_METADATA_LEN;
    if (0 == ppkt->trans_start.metadata.last_seg_num)
    {
        if (recv_data_len != pprov_info->prov_rx_pdu.total_data_len)
        {
            MESHX_WARN("segment(0) received length(%d) does not match required length(%d)", recv_data_len,
                       pprov_info->prov_rx_pdu.total_data_len);
            return -MESHX_ERR_LENGTH;
        }
    }
    else
    {
        if (recv_data_len != MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN)
        {
            MESHX_WARN("segment(0) received length(%d) does not match required length(%d)", recv_data_len,
                       pprov_info->prov_rx_pdu.total_data_len);
            return -MESHX_ERR_LENGTH;
        }
    }

    MESHX_BIT_FIELD_CLEAR(pprov_info->prov_rx_pdu.recv_seg, 0);
    memcpy(pprov_info->prov_rx_pdu.pdata, ppkt->trans_start.pdu, recv_data_len);

    int32_t ret = MESHX_SUCCESS;
    if (meshx_pb_adv_is_recv_all_trans_seg(pprov_info))
    {
        /* process provision pdu */
        ret = meshx_pb_adv_recv_prov_pdu(bearer, pprov_info->prov_rx_pdu.pdata, recv_data_len);
        if (MESHX_SUCCESS == ret)
        {
            /* TODO: send trans ack */
            pprov_info->acked_trans_num = ppkt->metadata.trans_num;
        }
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

    meshx_prov_info_t *pprov_info = NULL;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
#if MESHX_ROLE_DEVICE
    /* check link id first */
    if (link_id == prov_self.link_id)
    {
        pprov_info = &prov_self;
    }
#endif
    else
    {
#if MESHX_ROLE_PROVISIONER
        meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(link_id);
        if (NULL == pprov_dev)
        {
            pprov_info = &pprov_dev->prov_info;
        }
#endif
    }

    if (NULL == pprov_info)
    {
        MESHX_WARN("receive invalid link id: %d", link_id);
        return -MESHX_ERR_INVAL;
    }

    if (pprov_info->state < MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("invalid provision state: %d", pprov_info->state);
        return -MESHX_ERR_STATE;
    }

    if (pprov_info->acked_trans_num == ppkt->metadata.trans_num)
    {
        /* TODO: ignore data and acked again */
        MESHX_INFO("message(%d) has already been acked", ppkt->metadata.trans_num);
        return MESHX_SUCCESS;
    }

    if (pprov_info->rx_trans_num != ppkt->metadata.trans_num)
    {
        MESHX_WARN("receive dismatched trans num:%d-%d", pprov_info->rx_trans_num,
                   ppkt->metadata.trans_num);
        return -MESHX_ERR_FAIL;
    }

    if (NULL == pprov_info->prov_rx_pdu.pdata)
    {
        /* receive trans continue message before trans start message */
        MESHX_WARN("receive trans continue message before trans start message!");
        return -MESHX_ERR_FAIL;
    }

    if (!MESHX_IS_BIT_FIELD_SET(pprov_info->prov_rx_pdu.recv_seg,
                                ppkt->trans_continue.metadata.seg_index))
    {
        MESHX_WARN("segment %d has already been received!", ppkt->trans_continue.metadata.seg_index);
        return -MESHX_ERR_ALREADY;
    }

    uint8_t recv_data_len = len - MESHX_TRANS_CONTINUE_METADATA_LEN;
    if (ppkt->trans_continue.metadata.seg_index < pprov_info->prov_rx_pdu.last_seg_num)
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
        uint8_t required_len = pprov_info->prov_rx_pdu.total_data_len - MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN
                               - MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN * (pprov_info->prov_rx_pdu.last_seg_num - 1);
        if (recv_data_len != required_len)
        {
            MESHX_WARN("receive segment(%d) length(%d) does not match required length(%d)",
                       ppkt->trans_continue.metadata.seg_index, recv_data_len, required_len);
            return -MESHX_ERR_LENGTH;
        }
    }

    MESHX_BIT_FIELD_CLEAR(pprov_info->prov_rx_pdu.recv_seg, ppkt->trans_continue.metadata.seg_index);

    uint16_t offset = MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN + MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN *
                      (ppkt->trans_continue.metadata.seg_index - 1);
    memcpy(pprov_info->prov_rx_pdu.pdata + offset, ppkt->trans_continue.pdu, recv_data_len);

    int32_t ret = MESHX_SUCCESS;
    if (meshx_pb_adv_is_recv_all_trans_seg(pprov_info))
    {
        /* process provision pdu */
        ret = meshx_pb_adv_recv_prov_pdu(bearer, pprov_info->prov_rx_pdu.pdata,
                                         pprov_info->prov_rx_pdu.total_data_len);
        if (MESHX_SUCCESS == ret)
        {
            /* TODO: send trans ack */
            pprov_info->acked_trans_num = ppkt->metadata.trans_num;
        }
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

    int32_t ret = MESHX_SUCCESS;
    meshx_prov_info_t *pprov_info = NULL;
    uint32_t link_id = MESHX_BE32_TO_HOST(ppkt->metadata.link_id);
#if MESHX_ROLE_DEVICE
    /* check link id first */
    if (link_id == prov_self.link_id)
    {
        pprov_info = &prov_self;
    }
#endif
    else
    {
#if MESHX_ROLE_PROVISIONER
        meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(link_id);
        if (NULL == pprov_dev)
        {
            pprov_info = &pprov_dev->prov_info;
        }
#endif
    }

    if (NULL == pprov_info)
    {
        MESHX_WARN("receive invalid link id: %d", link_id);
        return -MESHX_ERR_INVAL;
    }

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
