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

#define MESHX_LINK_LOSS_TIME                    60000 /* unit is ms */
#define MESHX_LINK_RETRY_PERIOD                 200 /* unit is ms */

#define MESHX_TRANS_LOSS_TIME                   30000 /* unit is ms */
#define MESHX_TRANS_RETRY_PERIOD                500 /* unit is ms */

#define MESHX_BEARER_CTL_TRANS_NUM              0

#define MESHX_PROV_STATE_IDLE                   0
#define MESHX_PROV_STATE_LINK_OPENING           0x01
#define MESHX_PROV_STATE_LINK_OPENED            0x02
#define MESHX_PROV_STATE_INVITE                 0x03

typedef struct
{
    meshx_bearer_t bearer;
    uint16_t state;
    int8_t trans_num;
    uint32_t link_id;
    uint32_t retry_time;
    meshx_timer_t timer;
    uint8_t *prx_data;
    uint8_t rx_data_len;
    union
    {
        meshx_provision_invite_t invite;
    } tx_data;


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

static int32_t meshx_pb_adv_send(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                                 const uint8_t *pdata,
                                 uint8_t len)
{
    uint8_t data_len = 0;
    int32_t ret = MESHX_SUCCESS;
    if (len <= MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN)
    {
        /* no segment */
        meshx_pb_adv_pkt_t pb_adv_pkt;
        pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
        pb_adv_pkt.metadata.trans_num = trans_num;
        pb_adv_pkt.trans_start.metadata.gpcf = MESHX_GPCF_TRANS_START;
        pb_adv_pkt.trans_start.metadata.seg_num = 0;
        pb_adv_pkt.trans_start.metadata.total_len = MESHX_HOST_TO_BE16(len);
        pb_adv_pkt.trans_start.metadata.fcs = meshx_3gpp_crc(pdata, len);
        memcpy(pb_adv_pkt.trans_start.pdu, pdata, len);

        data_len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_start_metadata_t) + len;
        ret = meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                                data_len);
    }
    else
    {
        /* segmented */
#if 0
        uint8_t segment_num = ((len - MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN) +
                               MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN - 1) / MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN + 1;

        ASSERT(segment_num < 0x07);
#endif
    }

    return ret;
}

#if MESHX_ROLE_PROVISIONER
static int32_t pb_adv_link_open(meshx_bearer_t bearer, uint32_t link_id, meshx_dev_uuid_t dev_uuid)
{
    uint8_t len = 0;
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_OPEN;
    memcpy(pb_adv_pkt.bearer_ctl.link_open.dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t));
    len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t) + sizeof(
              meshx_pb_adv_link_open_t);

    MESHX_INFO("link opening: %d", link_id);
    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV,
                             (const uint8_t *)&pb_adv_pkt, len);
}
#endif

#if MESHX_ROLE_DEVICE
static int32_t pb_adv_link_ack(meshx_bearer_t bearer, uint32_t link_id)
{
    uint8_t len = 0;
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_ACK;
    len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t) + sizeof(
              meshx_pb_adv_link_ack_t);

    MESHX_INFO("link ack: %d", link_id);

    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV,
                             (const uint8_t *)&pb_adv_pkt, len);
}
#endif

static int32_t pb_adv_link_close(meshx_bearer_t bearer, uint32_t link_id, uint8_t reason)
{
    uint8_t len = 0;
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = MESHX_HOST_TO_BE32(link_id);
    pb_adv_pkt.metadata.trans_num = MESHX_BEARER_CTL_TRANS_NUM;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_CLOSE;
    pb_adv_pkt.bearer_ctl.link_close.reason = reason;
    len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t) + sizeof(
              meshx_pb_adv_link_close_t);

    MESHX_INFO("link close: %d", link_id);
    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                             len);
}

#if MESHX_ROLE_PROVISIONER
static int32_t pb_adv_invite(meshx_bearer_t bearer, uint32_t link_id, uint8_t trans_num,
                             meshx_provision_invite_t invite)
{
    MESHX_INFO("invite: %d", invite);
    meshx_provision_pdu_t prov_pdu;
    prov_pdu.metadata.type = MESHX_PROVISION_TYPE_INVITE;
    prov_pdu.invite = invite;
    return meshx_pb_adv_send(bearer, link_id, trans_num, (const uint8_t *)&prov_pdu,
                             sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_invite_t));
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

    if (NULL != pdev->prov_info.prx_data)
    {
        meshx_free(pdev->prov_info.prx_data);
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
                      pprov_dev->prov_info.trans_num, pprov_dev->prov_info.tx_data.invite);
        pprov_dev->prov_info.retry_time += MESHX_TRANS_RETRY_PERIOD;
        if (pprov_dev->prov_info.retry_time > MESHX_TRANS_LOSS_TIME)
        {
            MESHX_ERROR("provision failed: receive no ack of state(0x%04x)", pprov_dev->prov_info.state);
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
    pprov_dev->prov_info.link_id = ABS(meshx_rand());
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
    pprov_dev->prov_info.trans_num ++;
    pb_adv_invite(bearer, link_id, pprov_dev->prov_info.trans_num, invite);

    return MESHX_SUCCESS;
}

static int32_t meshx_pb_adv_recv_link_open(meshx_bearer_t bearer, const meshx_pb_adv_pkt_t *ppkt)
{
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

    /* link ack */
    meshx_pb_adv_link_ack(bearer, prov_self.link_id);

    return MESHX_SUCCESS;
}

static int32_t meshx_pb_adv_recv_link_ack(meshx_bearer_t bearer, const meshx_pb_adv_pkt_t *ppkt)
{
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
        return -MESHX_ERR_INVAL_STATE;
    }

    MESHX_INFO("link established with id %d", link_id);

    pprov_dev->prov_info.state = MESHX_PROV_STATE_LINK_OPENED;
    pprov_dev->prov_info.trans_num = -1;
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

static int32_t meshx_pb_adv_recv_bearer_ctl(meshx_bearer_t bearer, const meshx_pb_adv_pkt_t *ppkt)
{
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
        ret = meshx_pb_adv_recv_link_open(bearer, ppkt);
        break;
#endif
#if MESHX_ROLE_PROVISIONER
    case MESHX_BEARER_LINK_ACK:
        ret = meshx_pb_adv_recv_link_ack(bearer, ppkt);
        break;
#endif
    case MESHX_BEARER_LINK_CLOSE:
        MESHX_INFO("recv link close");
        break;
    default:
        MESHX_WARN("invalid bearer opcode: %d", ppkt->bearer_ctl.metadata.bearer_opcode);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

static int32_t meshx_pb_adv_recv_trans_start(meshx_bearer_t bearer, const meshx_pb_adv_pkt_t *ppkt)
{
    int32_t ret = MESHX_SUCCESS;
#if MESHX_ROLE_DEVICE
    /* check link id first */
    if (ppkt->metadata.link_id == prov_self.link_id)
    {

    }
#endif

#if MESHX_ROLE_PROVISIONER
    meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(ppkt->metadata.link_id);
    if (NULL == pprov_dev)
    {

    }
#endif

#if 0
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = link_id;
    pb_adv_pkt.metadata.trans_num = trans_num;
    pb_adv_pkt.trans_start.metadata.gpcf = MESHX_GPCF_TRANS_START;
    pb_adv_pkt.trans_start.metadata.seg_num = 0;
    pb_adv_pkt.trans_start.metadata.total_len = len;
    pb_adv_pkt.trans_start.metadata.fcs = meshx_3gpp_crc(pdata, len);
    memcpy(pb_adv_pkt.trans_start.pdu, pdata, len);
#endif

    return ret;
}

static int32_t meshx_pb_adv_recv_trans_continue(meshx_bearer_t bearer,
                                                const meshx_pb_adv_pkt_t *ppkt)
{
    int32_t ret = MESHX_SUCCESS;
#if MESHX_ROLE_DEVICE
    /* check link id first */
    if (ppkt->metadata.link_id == prov_self.link_id)
    {

    }
#endif
    else
    {
        meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(ppkt->metadata.link_id);
        if (NULL == pprov_dev)
        {

        }
    }

    return ret;
}

static int32_t meshx_pb_adv_recv_trans_ack(meshx_bearer_t bearer, const meshx_pb_adv_pkt_t *ppkt)
{
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
        MESHX_INFO("recv trans start");
        ret = meshx_pb_adv_recv_trans_start(bearer, ppb_adv_pkt);
        break;
    case MESHX_GPCF_TRANS_ACK:
        ret = meshx_pb_adv_recv_trans_ack(bearer, ppb_adv_pkt);
        break;
    case MESHX_GPCF_TRANS_CONTINUE:
        ret = meshx_pb_adv_recv_trans_continue(bearer, ppb_adv_pkt);
        break;
    case MESHX_GPCF_BEARER_CTL:
        ret = meshx_pb_adv_recv_bearer_ctl(bearer, ppb_adv_pkt);
        break;
    default:
        MESHX_WARN("invalid gpcf: %d", ppb_adv_pkt->pdu.gpcf);
        break;
    }
    return ret;
}