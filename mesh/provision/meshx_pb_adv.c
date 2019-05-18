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

#define MESHX_LINK_IDLE_TIME                10000 /* unit is ms */
#define MESHX_LINK_RETRY_PERIOD             200 /* unit is ms */

#define MESHX_PROV_STATE_IDLE               0
#define MESHX_PROV_STATE_LINK_OPENING       1
#define MESHX_PROV_STATE_LINK_OPENED        2
#define MESHX_PROV_STATE_INVITE             3

typedef struct
{
    meshx_bearer_t bearer;
    uint8_t state;
    uint32_t link_id;
    uint32_t link_idle_time;
    meshx_timer_t timer;
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

#if MESHX_ROLE_PROVISIONER
void meshx_pb_adv_devs_timeout_handler(void *pargs)
{
    meshx_prov_dev_t *pprov_dev = pargs;
    switch (pprov_dev->prov_info.state)
    {
    case MESHX_PROV_STATE_LINK_OPENING:
        meshx_pb_adv_link_open(pprov_dev->prov_info.bearer, pprov_dev->prov_info.link_id, pprov_dev->uuid);
        pprov_dev->prov_info.link_idle_time += MESHX_LINK_RETRY_PERIOD;
        if (pprov_dev->prov_info.link_idle_time > MESHX_LINK_IDLE_TIME)
        {
            MESHX_ERROR("provision failed: receive no ack from device uuid!");
            MESHX_DUMP_ERROR(pprov_dev->uuid, sizeof(meshx_dev_uuid_t));
            meshx_list_remove(&pprov_dev->node);
            meshx_timer_delete(pprov_dev->prov_info.timer);
            meshx_free(pprov_dev);
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

int32_t meshx_pb_adv_link_open(meshx_bearer_t bearer, uint32_t link_id, meshx_dev_uuid_t dev_uuid)
{
    int32_t ret = MESHX_SUCCESS;
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
        ret = meshx_timer_create(&pprov_dev->prov_info.timer, MESHX_TIMER_MODE_REPEATED,
                                 meshx_pb_adv_devs_timeout_handler, pprov_dev);
        if (MESHX_SUCCESS != ret)
        {
            meshx_free(pprov_dev);
            MESHX_ERROR("link open failed: create timer failed!");
            return ret;
        }
    }
    else
    {
        /* existing provision deivice */
        if (pprov_dev->prov_info.state > MESHX_PROV_STATE_LINK_OPENING)
        {
            MESHX_WARN("device is already in provision procedure");
            return -MESHX_ERR_BUSY;
        }
    }

    pprov_dev->prov_info.bearer = bearer;
    pprov_dev->prov_info.link_id = link_id;
    memcpy(pprov_dev->uuid, dev_uuid, sizeof(meshx_dev_uuid_t));

    uint8_t len = 0;
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = pprov_dev->prov_info.link_id;
    pb_adv_pkt.metadata.trans_num = 0;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_OPEN;
    memcpy(pb_adv_pkt.bearer_ctl.link_open.dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t));
    len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t) + sizeof(
              meshx_pb_adv_link_open_t);

    MESHX_INFO("link opening: %d", pprov_dev->prov_info.link_id);

    ret = meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV,
                            (const uint8_t *)&pb_adv_pkt,
                            len);
    if (MESHX_SUCCESS == ret)
    {
        meshx_list_append(&prov_devs, &pprov_dev->node);
        pprov_dev->prov_info.state = MESHX_PROV_STATE_LINK_OPENING;
        meshx_timer_start(pprov_dev->prov_info.timer, MESHX_LINK_RETRY_PERIOD);
    }
    else
    {
        meshx_timer_delete(pprov_dev->prov_info.timer);
        meshx_free(pprov_dev);
    }

    return ret;
}
#endif

#if MESHX_ROLE_DEVICE
int32_t meshx_pb_adv_link_ack(meshx_bearer_t bearer, uint32_t link_id)
{
    uint8_t len = 0;
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = link_id;
    pb_adv_pkt.metadata.trans_num = 0;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_ACK;
    len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t) + sizeof(
              meshx_pb_adv_link_ack_t);

    MESHX_INFO("link ack: %d", link_id);
    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                             len);
}
#endif

int32_t meshx_pb_adv_link_close(meshx_bearer_t bearer, uint32_t link_id, uint8_t reason)
{
    uint8_t len = 0;
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = link_id;
    pb_adv_pkt.metadata.trans_num = 0;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GPCF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_CLOSE;
    pb_adv_pkt.bearer_ctl.link_close.reason = reason;
    len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t) + sizeof(
              meshx_pb_adv_link_close_t);

    MESHX_INFO("link close: %d", link_id);
    return meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                             len);
}

static int32_t meshx_pb_adv_send(meshx_bearer_t bearer, uint32_t link_id, const uint8_t *pdata,
                                 uint8_t len)
{
    MESHX_ASSERT(len <= MESHX_PB_ADV_PDU_MAX_LEN);

    uint8_t data_len = 0;
    int32_t ret = MESHX_SUCCESS;
    if (len <= MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN)
    {
        /* no segment */
        meshx_pb_adv_pkt_t pb_adv_pkt;
        pb_adv_pkt.metadata.link_id = link_id;
        pb_adv_pkt.metadata.trans_num = 0;
        pb_adv_pkt.trans_start.metadata.gpcf = MESHX_GPCF_TRANS_START;
        pb_adv_pkt.trans_start.metadata.seg_num = 0;
        pb_adv_pkt.trans_start.metadata.total_len = len;
        pb_adv_pkt.trans_start.metadata.fcs = meshx_3gpp_crc(pdata, len);
        memcpy(pb_adv_pkt.trans_start.pdu, pdata, len);

        data_len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_trans_start_metadata_t) + len;
        ret = meshx_bearer_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt,
                                data_len);
    }
    else
    {
        /* segmented */
        //uint8_t segment_num = ((len - MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN) + MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN - 1) / MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN + 1;

    }

    return ret;
}

int32_t meshx_pb_adv_invite(meshx_bearer_t bearer, uint32_t link_id,
                            meshx_provision_invite_t invite)
{
    MESHX_INFO("invite: %d", invite);
    return meshx_pb_adv_send(bearer, link_id, (const uint8_t *)&invite,
                             sizeof(meshx_provision_invite_t));
}

static int32_t meshx_pb_adv_recv_link_open(meshx_bearer_t bearer, const meshx_pb_adv_pkt_t *ppkt)
{
    /* check uuid */
    meshx_dev_uuid_t dev_uuid;
    meshx_get_device_uuid(dev_uuid);
    if (0 != memcmp(dev_uuid, ppkt->bearer_ctl.link_open.dev_uuid, sizeof(meshx_dev_uuid_t)))
    {
        MESHX_INFO("receive dismatched uuid");
        return -MESHX_ERR_DIFF;
    }

    /* check state */
    if (prov_self.state > MESHX_PROV_STATE_LINK_OPENED)
    {
        MESHX_WARN("device is already in provisioning procedure: %d-%d", prov_self.state,
                   prov_self.link_id);
        return -MESHX_ERR_BUSY;
    }

    /* check state and link id */
    if ((MESHX_PROV_STATE_LINK_OPENED == prov_self.state) &&
        (prov_self.link_id != ppkt->metadata.link_id))
    {
        MESHX_INFO("receive dismatched link id: %d-%d", prov_self.link_id, ppkt->metadata.link_id);
        return -MESHX_ERR_DIFF;
    }


    MESHX_INFO("receive link open: %d", ppkt->metadata.link_id);
    prov_self.link_id = ppkt->metadata.link_id;

    /* link ack */
    int32_t ret = meshx_pb_adv_link_ack(bearer, prov_self.link_id);
    if (MESHX_SUCCESS == ret)
    {
        prov_self.state = MESHX_PROV_STATE_LINK_OPENED;
    }

    return ret;
}

static int32_t meshx_pb_adv_recv_link_ack(meshx_bearer_t bearer, const meshx_pb_adv_pkt_t *ppkt)
{
    int32_t ret = MESHX_SUCCESS;

    meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(ppkt->metadata.link_id);
    if (NULL == pprov_dev)
    {
        MESHX_WARN("can't find provision device with link id %d", ppkt->metadata.link_id);
    }

    MESHX_INFO("link established with id %d", ppkt->metadata.link_id);

    /* start invite */
    meshx_provision_invite_t invite = {0};
    /* TODO: get invite from app */
    ret = meshx_pb_adv_invite(bearer, ppkt->metadata.link_id, invite);
    if (MESHX_SUCCESS == ret)
    {
        pprov_dev->prov_info.state = MESHX_PROV_STATE_INVITE;
    }
    else
    {
        pprov_dev->prov_info.state = MESHX_PROV_STATE_LINK_OPENED;
    }


    return ret;
}

static int32_t meshx_pb_adv_recv_bearer_ctl(meshx_bearer_t bearer, const meshx_pb_adv_pkt_t *ppkt)
{
    if (ppkt->metadata.trans_num != 0)
    {
        MESHX_WARN("the transition number of bearer control must be 0: %d", ppkt->metadata.trans_num);
        return -MESHX_ERR_INVAL;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (ppkt->bearer_ctl.metadata.bearer_opcode)
    {
    case MESHX_BEARER_LINK_OPEN:
        ret = meshx_pb_adv_recv_link_open(bearer, ppkt);
        break;
    case MESHX_BEARER_LINK_ACK:
        ret = meshx_pb_adv_recv_link_ack(bearer, ppkt);
        break;
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
    /* check link id first */
    if (ppkt->metadata.link_id == prov_self.link_id)
    {

    }
    else
    {
        meshx_prov_dev_t *pprov_dev = meshx_find_prov_dev_by_link_id(ppkt->metadata.link_id);
        if (NULL == pprov_dev)
        {

        }
    }

    return ret;
}

static int32_t meshx_pb_adv_recv_trans_continue(meshx_bearer_t bearer,
                                                const meshx_pb_adv_pkt_t *ppkt)
{
    int32_t ret = MESHX_SUCCESS;
    /* check link id first */
    if (ppkt->metadata.link_id == prov_self.link_id)
    {

    }
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