/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MEHSX_PROVISION"
#include "meshx_trace.h"
#include "meshx_provision.h"
#include "meshx_errno.h"
#include "meshx_pb_adv.h"


int32_t meshx_provision_init(void)
{
    meshx_pb_adv_init();
    return MESHX_SUCCESS;
}

int32_t meshx_provision_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    int32_t ret = MESHX_SUCCESS;
    switch (bearer.type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_receive(bearer, pdata, len);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer(%d)", bearer.bearer);
        ret = -MESHX_ERR_INVAL_BEARER;
        break;
    }
    return ret;
}

int32_t meshx_provision_link_open(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid)
{
    if (MESHX_BEARER_TYPE_ADV != bearer.type)
    {
        MESHX_WARN("link open message can only send on advertising bearer!");
        return -MESHX_ERR_INVAL_BEARER;
    }

    return meshx_pb_adv_link_open(bearer, dev_uuid);
}

int32_t meshx_provision_link_ack(meshx_bearer_t bearer)
{
    if (MESHX_BEARER_TYPE_ADV != bearer.type)
    {
        MESHX_WARN("link ack message can only send on advertising bearer!");
        return -MESHX_ERR_INVAL_BEARER;
    }

    return meshx_pb_adv_link_ack(bearer);
}

int32_t meshx_provision_link_close(meshx_bearer_t bearer, uint8_t reason)
{
    if (MESHX_BEARER_TYPE_ADV != bearer.type)
    {
        MESHX_WARN("link close message can only send on advertising bearer!");
        return -MESHX_ERR_INVAL_BEARER;
    }

    return meshx_pb_adv_link_close(bearer, reason);
}

int32_t meshx_provision_invite(meshx_bearer_t bearer, meshx_provision_invite_t invite)
{
    int32_t ret = MESHX_SUCCESS;
    switch (bearer.type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_invite(bearer, invite);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer: bearer");
        ret = -MESHX_ERR_INVAL_BEARER;
        break;
    }

    return ret;
}

