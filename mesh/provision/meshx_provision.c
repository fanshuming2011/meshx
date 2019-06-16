/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MEHSX_PROVISION"
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

static meshx_provision_callback_t prov_cb;

int32_t meshx_provision_init(meshx_provision_callback_t pcb)
{
    meshx_pb_adv_init(pcb);
    prov_cb = pcb;
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

int32_t meshx_provision_link_close(meshx_provision_dev_t prov_dev, uint8_t reason)
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
#if MESHX_ROLE_DEVICE
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
            meshx_provision_invite_t invite = pprov_pdu->invite;
            if (NULL != prov_cb)
            {
                ret = prov_cb(prov_dev, MESHX_PROVISION_CB_TYPE_INVITE, &invite);
            }
        }
        break;
#endif
#if MESHX_ROLE_PROVISIONER
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
            meshx_provision_capabilites_t cap = pprov_pdu->capabilites;
            if (NULL != prov_cb)
            {
                ret = prov_cb(prov_dev, MESHX_PROVISION_CB_TYPE_CAPABILITES, &cap);
            }
        }
        break;
#endif
#if MESHX_ROLE_DEVICE
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
            meshx_provision_start_t start = pprov_pdu->start;
            if (NULL != prov_cb)
            {
                ret = prov_cb(prov_dev, MESHX_PROVISION_CB_TYPE_START, &start);
            }
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
#if MESHX_ROLE_DEVICE
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


