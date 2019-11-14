/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx_beacon.h"
#define MESHX_TRACE_MODULE "MESHX_BEACON"
#include "meshx_trace.h"
#include "meshx_beacon.h"
#include "meshx_errno.h"
#include "meshx_bearer_internal.h"
#include "meshx_timer.h"
#include "meshx_node.h"
#include "meshx_async_internal.h"
#include "meshx_beacon_internal.h"
#include "meshx_notify.h"
#include "meshx_notify_internal.h"
#include "meshx_node_internal.h"
#include "meshx_iv_index_internal.h"
#include "meshx_iv_index.h"
#include "meshx_security.h"
#include "meshx_key.h"
#include "meshx_assert.h"

static meshx_timer_t beacon_timer;
static uint16_t meshx_oob_info;
static bool meshx_uri_hash_exists;
static uint32_t meshx_uri_hash;

static void meshx_beacon_timer_timeout(void *pargs)
{
    meshx_async_msg_t async_msg;
    async_msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_BEACON;
    async_msg.pdata = pargs;
    async_msg.data_len = 0;

    meshx_async_msg_send(&async_msg);
}

static void meshx_beacon_send_udb(meshx_bearer_t bearer)
{
    /* send udb */
    meshx_udb_t udb;
    uint8_t len = sizeof(meshx_udb_t);
    udb.type = MESHX_BEACON_TYPE_UDB;
    memcpy(udb.dev_uuid, meshx_node_params.config.dev_uuid, sizeof(meshx_dev_uuid_t));
    udb.oob_info = meshx_oob_info;
    if (meshx_uri_hash_exists)
    {
        udb.uri_hash = meshx_uri_hash;
        len = sizeof(meshx_udb_t);
    }
    else
    {
        len = MESHX_OFFSET_OF(meshx_udb_t, uri_hash);
    }
    MESHX_INFO("udb send:");
    MESHX_DUMP_INFO(&udb, len);
    meshx_bearer_adv_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_BEACON, (const uint8_t *)&udb, len);
}

static void meshx_beacon_send_snb(meshx_bearer_t bearer)
{
    meshx_snb_t snb;
    snb.type = MESHX_BEACON_TYPE_SNB;
    snb.flag.key_refresh = 0;
    snb.flag.iv_update = meshx_iv_update_state_get();
    snb.flag.rsvd = 0;
    snb.iv_index = meshx_iv_index_get();

    uint8_t snb_auth[16];
    meshx_net_key_t *pnet_key = NULL;
    const meshx_net_key_value_t *pnet_key_value = NULL;
    meshx_net_key_traverse_start(&pnet_key);
    while (NULL != pnet_key)
    {
        pnet_key_value = &pnet_key->key_value[0];
        if (MESHX_KEY_STATE_PHASE2 == pnet_key->key_state)
        {
            pnet_key_value = &pnet_key->key_value[1];
        }
        memcpy(snb.net_id, pnet_key_value->net_id, sizeof(meshx_net_id_t));
        meshx_aes_cmac(pnet_key_value->beacon_key, ((uint8_t *)&snb) + 1, sizeof(meshx_snb_t) - 9,
                       snb_auth);
        memcpy(snb.auth_value, snb_auth, 8);

        /* send snb */
        MESHX_INFO("snb send:");
        MESHX_DUMP_INFO(&snb, sizeof(meshx_snb_t));
        meshx_bearer_adv_send(bearer, MESHX_BEARER_ADV_PKT_TYPE_BEACON, (const uint8_t *)&snb,
                              sizeof(meshx_snb_t));

        meshx_net_key_traverse_continue(&pnet_key);
    }
}

void meshx_beacon_async_handle_timeout(meshx_async_msg_t msg)
{
    if (MESHX_ADDRESS_UNASSIGNED == meshx_node_params.param.node_addr)
    {
        meshx_beacon_send_udb(msg.pdata);
    }
    else
    {
        meshx_beacon_send_snb(msg.pdata);
    }
}

int32_t meshx_beacon_send(meshx_bearer_t bearer, uint8_t beacon_type)
{
    if (MESHX_BEARER_TYPE_ADV != bearer->type)
    {
        MESHX_ERROR("beacon can only send on advertising bearer!");
        return -MESHX_ERR_INVAL;
    }
    int32_t ret = MESHX_SUCCESS;
    switch (beacon_type)
    {
    case MESHX_BEACON_TYPE_UDB:
        if (MESHX_ADDRESS_UNASSIGNED != meshx_node_params.param.node_addr)
        {
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            meshx_beacon_send_udb(bearer);
        }
        break;
    case MESHX_BEACON_TYPE_SNB:
        if (MESHX_ADDRESS_UNASSIGNED == meshx_node_params.param.node_addr)
        {
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            meshx_beacon_send_snb(bearer);
        }
        break;
    default:
        MESHX_ERROR("invalid beacon type: %d", beacon_type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    return ret;
}

int32_t meshx_beacon_start(meshx_bearer_t bearer, uint8_t beacon_type, uint32_t interval)
{
    if ((NULL == bearer) || (MESHX_BEARER_TYPE_ADV != bearer->type))
    {
        MESHX_ERROR("can't start beacon, invlaid bearer");
        return -MESHX_ERR_INVAL;
    }
    int32_t ret = MESHX_SUCCESS;
    switch (beacon_type)
    {
    case MESHX_BEACON_TYPE_UDB:
        if (MESHX_ADDRESS_UNASSIGNED != meshx_node_params.param.node_addr)
        {
            MESHX_ERROR("node is provisioned, can't send udb!");
            ret = -MESHX_ERR_STATE;
        }
        break;
    case MESHX_BEACON_TYPE_SNB:
        if (MESHX_ADDRESS_UNASSIGNED == meshx_node_params.param.node_addr)
        {
            MESHX_ERROR("node is unprovisioned, can't send snb!");
            ret = -MESHX_ERR_STATE;
        }
        break;
    default:
        MESHX_ERROR("invalid beacon type: %d", beacon_type);
        ret = -MESHX_ERR_INVAL;
        break;
    }

    if (MESHX_SUCCESS == ret)
    {
        if (NULL == beacon_timer)
        {
            ret = meshx_timer_create(&beacon_timer, MESHX_TIMER_MODE_REPEATED, meshx_beacon_timer_timeout,
                                     bearer);
            if (MESHX_SUCCESS == ret)
            {
                meshx_timer_start(beacon_timer, interval);
            }
        }
        else
        {
            meshx_timer_start(beacon_timer, interval);
        }
    }

    return ret;
}

void meshx_beacon_stop(uint8_t beacon_type)
{
    if (NULL != beacon_timer)
    {
        meshx_timer_stop(beacon_timer);
        meshx_timer_delete(beacon_timer);
        beacon_timer = NULL;
    }
}

void meshx_beacon_set_oob_info(uint16_t oob_info)
{
    meshx_oob_info = oob_info;
}

void meshx_beacon_set_uri_hash(uint32_t uri_hash)
{
    meshx_uri_hash_exists = TRUE;
    meshx_uri_hash = uri_hash;
}

int32_t meshx_beacon_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len,
                             const meshx_adv_metadata_t *padv_metadata)
{
    MESHX_ASSERT(NULL != pdata);
    int32_t ret = MESHX_SUCCESS;
    uint8_t beacon_type = pdata[0];
    switch (beacon_type)
    {
    case MESHX_BEACON_TYPE_UDB:
        {
            const meshx_udb_t *pudb = (const meshx_udb_t *)pdata;
            meshx_notify_udb_t udb;
            uint8_t notify_len = sizeof(meshx_notify_udb_t);
            udb.padv_metadata = padv_metadata;
            memcpy(udb.dev_uuid, pudb->dev_uuid, sizeof(meshx_dev_uuid_t));
            udb.oob_info = pudb->oob_info;
            if (len == sizeof(meshx_udb_t))
            {
                udb.uri_hash = pudb->uri_hash;
            }
            else
            {
                notify_len -=  sizeof(uint32_t);
            }
            meshx_notify(bearer, MESHX_NOTIFY_TYPE_UDB, &udb, notify_len);
        }
        break;
    case MESHX_BEACON_TYPE_SNB:
        {
            /* find netkey first */
            const meshx_snb_t *psnb = (const meshx_snb_t *)pdata;
            meshx_net_key_t *pnet_key = NULL;
            meshx_net_key_traverse_start(&pnet_key);
            while (NULL != pnet_key)
            {
                uint8_t loop = 1;
                if ((MESHX_KEY_STATE_PHASE1 == pnet_key->key_state) ||
                    (MESHX_KEY_STATE_PHASE2 == pnet_key->key_state))
                {
                    loop = 2;
                }

                for (uint8_t i = 0; i < loop; ++i)
                {
                    if (0 == memcmp(pnet_key->key_value[i].net_id, psnb->net_id, sizeof(meshx_net_id_t)))
                    {
                        uint8_t snb_auth[16];
                        meshx_aes_cmac(pnet_key->key_value[i].beacon_key, ((uint8_t *)psnb) + 1, sizeof(meshx_snb_t) - 9,
                                       snb_auth);
                        if (0 == memcmp(psnb->auth_value, snb_auth, 8))
                        {
                            if (psnb->flag.key_refresh)
                            {
                                if (MESHX_KEY_STATE_NORMAL == pnet_key->key_state)
                                {
                                    MESHX_ERROR("current state is normal, can't enter phase2");
                                    ret = -MESHX_ERR_STATE;
                                    goto FINISH;
                                }
                                pnet_key->key_state = MESHX_KEY_STATE_PHASE2;
                                meshx_app_key_state_transit(pnet_key->key_index, MESHX_KEY_STATE_PHASE2);
                            }
                            else
                            {
                                if (1 == i)
                                {
                                    /* use new key, skip state phase2 */
                                    pnet_key->key_state = MESHX_KEY_STATE_NORMAL;
                                    /* revoke old keys */
                                    pnet_key->key_value[0] = pnet_key->key_value[1];
                                    meshx_app_key_state_transit(pnet_key->key_index, MESHX_KEY_STATE_NORMAL);
                                }
                            }

                            meshx_iv_update_state_t iv_update_state = MESHX_IV_UPDATE_STATE_NORMAL;
                            if (psnb->flag.iv_update)
                            {
                                iv_update_state = MESHX_IV_UPDATE_STATE_IN_PROGRESS;
                            }
                            /* iv update */
                            meshx_iv_index_update(psnb->iv_index, iv_update_state);
                            goto FINISH;
                        }
                    }
                }

                meshx_net_key_traverse_continue(&pnet_key);
            }

FINISH:
            if (NULL == pnet_key)
            {
                MESHX_DEBUG("received snb is not in this network");
                ret = -MESHX_ERR_KEY;
            }
        }
        break;
    default:
        break;
    }

    return ret;
}
