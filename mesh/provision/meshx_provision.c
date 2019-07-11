/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MEHSX_PROVISION"
#include "meshx_trace.h"
#include "meshx_provision.h"
#include "meshx_provision_internal.h"
#include "meshx_errno.h"
#include "meshx_pb_adv.h"
#include "meshx_misc.h"
#include "meshx_config.h"
#include "meshx_node.h"
#include "meshx_bearer_internal.h"
#include "meshx_list.h"
#include "meshx_notify.h"
#include "meshx_notify_internal.h"


int32_t meshx_provision_init(void)
{
    meshx_pb_adv_init();
    return MESHX_SUCCESS;
}

int32_t meshx_provision_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    int32_t ret = MESHX_SUCCESS;
    switch (bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_receive(bearer, pdata, len);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer type(%d)", bearer->type);
        ret = -MESHX_ERR_INVAL;
        break;
    }
    return ret;
}

meshx_provision_dev_t meshx_provision_create_device(meshx_bearer_t bearer,
                                                    meshx_dev_uuid_t dev_uuid)
{
    if (NULL == bearer)
    {
        MESHX_ERROR("bearer value is NULL!");
        return NULL;
    }

    meshx_provision_dev_t prov_dev = NULL;
    switch (bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        prov_dev = meshx_pb_adv_create_device(bearer, dev_uuid);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer type(%d)", bearer->type);
        break;
    }

    return prov_dev;
}

static void meshx_provision_delete_device(meshx_provision_dev_t prov_dev)
{
    if (NULL == prov_dev)
    {
        return;
    }

    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        meshx_pb_adv_delete_device(prov_dev);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_ERROR("invalid bearer type: %d", prov_dev->bearer->type);
        break;
    }
}

int32_t meshx_provision_link_open(meshx_provision_dev_t prov_dev)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("provision device value is NULL");
        return -MESHX_ERR_INVAL;
    }

    if (MESHX_BEARER_TYPE_ADV != prov_dev->bearer->type)
    {
        MESHX_WARN("link open message can only send on advertising bearer!");
        return -MESHX_ERR_INVAL;
    }

    return meshx_pb_adv_link_open(prov_dev);
}

int32_t meshx_provision_link_close(meshx_provision_dev_t prov_dev,
                                   meshx_provision_link_close_reason_t reason)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("provision device value is NULL");
        return -MESHX_ERR_INVAL;
    }
    return meshx_pb_adv_link_close(prov_dev, reason);
}

int32_t meshx_provision_invite(meshx_provision_dev_t prov_dev,
                               meshx_provision_invite_t invite)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("provision device value is NULL");
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_LINK_OPENED) ||
        (prov_dev->state > MESHX_PROVISION_STATE_INVITE))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    if (MESHX_PROVISION_STATE_INVITE == prov_dev->state)
    {
        MESHX_WARN("already in invite procedure");
        return -MESHX_ERR_ALREADY;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_invite(prov_dev, invite);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer type: %d", prov_dev->bearer->type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

int32_t meshx_provision_capabilites(meshx_provision_dev_t prov_dev,
                                    const meshx_provision_capabilites_t *pcap)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("provision device value is NULL");
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_LINK_OPENED) ||
        (prov_dev->state > MESHX_PROVISION_STATE_CAPABILITES))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    if (MESHX_PROVISION_STATE_CAPABILITES == prov_dev->state)
    {
        MESHX_WARN("already in capabilites procedure");
        return -MESHX_ERR_ALREADY;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_capabilites(prov_dev, pcap);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer type: %d", prov_dev->bearer->type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

int32_t meshx_provision_start(meshx_provision_dev_t prov_dev,
                              const meshx_provision_start_t *pstart)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("provision device value is NULL");
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_LINK_OPENED) ||
        (prov_dev->state > MESHX_PROVISION_STATE_START))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    if (MESHX_PROVISION_STATE_START == prov_dev->state)
    {
        MESHX_WARN("already in start procedure");
        return -MESHX_ERR_ALREADY;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_start(prov_dev, pstart);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer type: %d", prov_dev->bearer->type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

int32_t meshx_provision_pdu_process(meshx_provision_dev_t prov_dev,
                                    const uint8_t *pdata, uint8_t len)
{
    const meshx_provision_pdu_t *pprov_pdu = (const meshx_provision_pdu_t *)pdata;
    int32_t ret = MESHX_SUCCESS;
    switch (pprov_pdu->metadata.type)
    {
#if MESHX_SUPPORT_ROLE_DEVICE
    case MESHX_PROVISION_TYPE_INVITE:
        if (len < sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_invite_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid ivnvite pdu length: %d", len);
            ret = -MESHX_ERR_LENGTH;
        }
        else
        {
            /* notify app invite value */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_INVITE;
            notify_prov.invite = pprov_pdu->invite;
            meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                         sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_invite_t));
        }
        break;
#endif
#if MESHX_SUPPORT_ROLE_PROVISIONER
    case MESHX_PROVISION_TYPE_CAPABILITES:
        if (len < sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_capabilites_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid capabilites pdu length: %d", len);
            ret = -MESHX_ERR_LENGTH;
        }
        else
        {
            /* notify app capabilites value */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_CAPABILITES;
            notify_prov.capabilites = pprov_pdu->capabilites;
            meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                         sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_capabilites_t));
        }
        break;
#endif
#if MESHX_SUPPORT_ROLE_DEVICE
    case MESHX_PROVISION_TYPE_START:
        if (len < sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_start_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid start pdu length: %d", len);
            ret = -MESHX_ERR_LENGTH;
        }
        else
        {
            /* notify app start value */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_START;
            notify_prov.start = pprov_pdu->start;
            meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                         sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_start_t));
        }
        break;
#endif
    case MESHX_PROVISION_TYPE_PUBLIC_KEY:
        break;
    case MESHX_PROVISION_TYPE_INPUT_COMPLETE:
        break;
    case MESHX_PROVISION_TYPE_CONFIRMATION:
        break;
    case MESHX_PROVISION_TYPE_RANDOM:
        break;
#if MESHX_SUPPORT_ROLE_DEVICE
    case MESHX_PROVISION_TYPE_DATA:
        break;
#endif
    case MESHX_PROVISION_TYPE_COMPLETE:
        break;
    case MESHX_PROVISION_TYPE_FAILED:
        break;
    default:
        /* TODO: provision failed: invalid pdu */

        break;
    }

    /* notify app state changed */
    return ret;
}

int32_t meshx_provision_handle_notify(meshx_bearer_t bearer, const meshx_notify_prov_t *pnotify,
                                      uint8_t len)
{
    bool prov_end = FALSE;
    switch (pnotify->metadata.notify_type)
    {
    case MESHX_PROV_NOTIFY_LINK_OPEN:
        if (MESHX_PROVISION_LINK_OPEN_SUCCESS == pnotify->link_open_result)
        {
#if MESHX_SUPPORT_ROLE_DEVICE
            /* stop pb-adv and pb-gatt */
            meshx_beacon_stop(MESHX_BEACON_TYPE_UDB);
#endif
        }
        else
        {
            /* provision failed */
            prov_end = TRUE;
        }
        break;
    case MESHX_PROV_NOTIFY_LINK_CLOSE:
        /* check node state */
        if (MESHX_NODE_PROVED == meshx_get_node_prov_state())
        {
            /* start snb */
            //meshx_beacon_start(adv_bearer, MESHX_BEACON_TYPE_UDB, 5000);
        }
        else
        {
            uint32_t udb_interval = 0;
            meshx_node_param_get(MESHX_NODE_PARAM_TYPE_UDB_INTERVAL, &udb_interval);
            /* start pb-adv and pb-gatt */
            meshx_beacon_start(bearer, MESHX_BEACON_TYPE_UDB, udb_interval);
        }
        prov_end = TRUE;
        break;
    default:
        break;
    }
    int32_t ret = meshx_notify(bearer, MESHX_NOTIFY_TYPE_PROV, pnotify, len);
    if (prov_end)
    {
        meshx_provision_delete_device(pnotify->metadata.prov_dev);
    }
    return ret;
}

