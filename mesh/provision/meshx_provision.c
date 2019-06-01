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
#include "meshx_errno.h"
#include "meshx_pb_adv.h"
#include "meshx_misc.h"
#include "meshx_config.h"
#include "meshx_node.h"
#include "meshx_bearer_internal.h"

static meshx_provision_callback_t prov_cb;

int32_t meshx_provision_init(meshx_provision_callback_t pcb)
{
    meshx_pb_adv_init();
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

int32_t meshx_provision_link_open(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid)
{
    if (NULL == bearer)
    {
        MESHX_ERROR("bearer value is NULL");
        return -MESHX_ERR_INVAL;
    }

    if (MESHX_BEARER_TYPE_ADV != bearer->type)
    {
        MESHX_WARN("link open message can only send on advertising bearer!");
        return -MESHX_ERR_INVAL;
    }

    return meshx_pb_adv_link_open(bearer, dev_uuid);
}

int32_t meshx_provision_link_close(meshx_dev_uuid_t dev_uuid, uint8_t reason)
{
    return meshx_pb_adv_link_close(dev_uuid, reason);
}

int32_t meshx_provision_invite(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid,
                               meshx_provision_invite_t invite)
{
    if (NULL == bearer)
    {
        MESHX_ERROR("bearer value is NULL");
        return -MESHX_ERR_INVAL;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_invite(dev_uuid, invite);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer type: %d", bearer->type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

void meshx_provision_state_changed(meshx_dev_uuid_t dev_uuid, uint8_t new_state, uint8_t old_state)
{
    /* notify app state changed */
    meshx_provision_state_changed_t state_changed;
    state_changed.new_state = new_state;
    state_changed.old_state = old_state;
    memcpy(state_changed.dev_uuid, dev_uuid, sizeof(meshx_dev_uuid_t));
    int32_t ret = MESHX_SUCCESS;
    if (NULL != prov_cb)
    {
        ret = prov_cb(MESHX_PROVISION_CB_TYPE_STATE_CHANGED, &state_changed);
    }

    bool is_myself = FALSE;
    meshx_dev_uuid_t uuid_self;
    meshx_get_device_uuid(uuid_self);
    if (0 == memcmp(uuid_self, dev_uuid, sizeof(meshx_dev_uuid_t)))
    {
        is_myself = TRUE;
    }

    if (-MESHX_ERR_STOP != ret)
    {
        switch (new_state)
        {
        case MESHX_PROVISION_STATE_LINK_OPENED:
            if (!is_myself)
            {
                if (-MESHX_ERR_STOP != ret)
                {
                    meshx_provision_invite_t invite = {0};
                    meshx_pb_adv_invite(dev_uuid, invite);
                }
            }
            break;
        case MESHX_PROVISION_STATE_INVITE:
            break;
        default:
            break;
        }
    }
}

int32_t meshx_provision_pdu_process(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid,
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
            MESHX_ERROR("invalid ivnvite pdu length: %d", len);
            ret = -MESHX_ERR_LENGTH;
        }
        else
        {
            /* notify app invite value */
            meshx_provision_invite_t invite = pprov_pdu->invite;
            if (NULL != prov_cb)
            {
                prov_cb(MESHX_PROVISION_CB_TYPE_SET_INVITE, &invite);
            }
        }
        break;
#endif
#if MESHX_ROLE_PROVISIONER
    case MESHX_PROVISION_TYPE_CAPABILITES:
        break;
    case MESHX_PROVISION_TYPE_START:
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
    case MESHX_PROVISION_TYPE_DATA:
        break;
    case MESHX_PROVISION_TYPE_COMPLETE:
        break;
    case MESHX_PROVISION_TYPE_FAILED:
        break;
    default:
        break;
    }

    /* notify app state changed */
    return ret;
}
