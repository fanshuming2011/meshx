/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MESHX_PB_ADV"
#include "meshx_trace.h"
#include "meshx_pb_adv.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_list.h"

typedef struct
{
    uint32_t link_id;
    meshx_dev_uuid_t dev_uuid;
    meshx_list_t node;
} meshx_proving_devs_t;

static meshx_list_t proving_devs_list;

int32_t meshx_pb_adv_prov_init(void)
{
    meshx_list_init_head(&proving_devs_list);
    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_link_open(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid)
{
    meshx_list_t *pnode;
    meshx_proving_devs_t *pproving_dev;
    meshx_list_foreach(pnode, &proving_devs_list)
    {
        pproving_dev = MESHX_CONTAINER_OF(pnode, meshx_proving_devs_t, node);
        if (0 == memcmp(pproving_dev->dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t)))
        {
            MESHX_INFO("device is already in provision procedure");
            return -MESHX_ERR_ALREADY;
        }
    }

    pproving_dev = meshx_malloc(sizeof(meshx_proving_devs_t));
    if (NULL == pproving_dev)
    {
        MESHX_WARN("link open failed: out of memory!");
        return -MESHX_ERR_NO_MEM;
    }
    memcpy(pproving_dev->dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t));
    meshx_list_append(&proving_devs_list, &pproving_dev->node);
    pproving_dev->link_id = meshx_list_length(&proving_devs_list);

    uint8_t len = 0;
    meshx_pb_adv_pkt_t pb_adv_pkt;
    pb_adv_pkt.metadata.link_id = pproving_dev->link_id;
    pb_adv_pkt.metadata.trans_num = 0;
    pb_adv_pkt.bearer_ctl.metadata.gpcf = MESHX_GCPF_BEARER_CTL;
    pb_adv_pkt.bearer_ctl.metadata.bearer_opcode = MESHX_BEARER_LINK_OPEN;
    memcpy(pb_adv_pkt.bearer_ctl.link_open.dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t));
    len = sizeof(meshx_pb_adv_metadata_t) + sizeof(meshx_pb_adv_bearer_ctl_metadata_t) + sizeof(
              meshx_pb_adv_link_open_t);

    MESHX_INFO("link opening: %d", pproving_dev->link_id);
    return meshx_bearer_send(bearer, MESHX_BEARER_PKT_TYPE_PB_ADV, (const uint8_t *)&pb_adv_pkt, len);
}

int32_t meshx_pb_adv_link_ack(meshx_bearer_t bearer)
{
    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_link_close(meshx_bearer_t bearer, uint8_t reason)
{
    return MESHX_SUCCESS;
}

int32_t meshx_pb_adv_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    MESHX_ASSERT(MESHX_BEARER_TYPE_ADV == bearer.type);
    //meshx_pb_adv_pkt_t *ppb_adv_pkt = pdata;
    return MESHX_SUCCESS;
}