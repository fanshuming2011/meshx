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
#include "meshx_security.h"
#include "meshx_mem.h"

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
                                                    meshx_dev_uuid_t dev_uuid,
                                                    meshx_role_t role)
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
        prov_dev = meshx_pb_adv_create_device(bearer, dev_uuid, role);
        break;
    case MESHX_BEARER_TYPE_GATT:
        break;
    default:
        MESHX_WARN("invalid bearer type(%d)", bearer->type);
        break;
    }

    return prov_dev;
}

int32_t meshx_provision_make_key(meshx_provision_dev_t prov_dev)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("invalid provision device!");
        return -MESHX_ERR_INVAL;
    }
    return meshx_ecc_make_key(prov_dev->public_key, prov_dev->private_key);
}

bool meshx_provision_validate_public_key(const meshx_provision_public_key_t *pkey)
{
    return meshx_ecc_validate_public_key((const uint8_t *)pkey);
}

int32_t meshx_provision_get_local_public_key(meshx_provision_dev_t prov_dev,
                                             meshx_provision_public_key_t *pkey)
{
    if ((NULL == pkey) || (NULL == prov_dev))
    {
        MESHX_ERROR("invalid parameter: prov_dev 0x%x, pkey 0x%x!", prov_dev, pkey);
        return -MESHX_ERR_INVAL;
    }

    memcpy(pkey, prov_dev->public_key, sizeof(meshx_provision_public_key_t));

    return MESHX_SUCCESS;
}

int32_t meshx_provision_set_remote_public_key(meshx_provision_dev_t prov_dev,
                                              const meshx_provision_public_key_t *pkey)
{
    if ((NULL == pkey) || (NULL == prov_dev))
    {
        MESHX_ERROR("invalid parameter: prov_dev 0x%x, pkey 0x%x!", prov_dev, pkey);
        return -MESHX_ERR_INVAL;
    }

    memcpy(prov_dev->public_key, pkey, sizeof(meshx_provision_public_key_t));

    return MESHX_SUCCESS;
}

int32_t meshx_provision_generate_confirmation(meshx_provision_dev_t prov_dev,
                                              uint8_t auth_value[16])
{
    uint16_t cfm_inputs_len = sizeof(meshx_provision_invite_t) + sizeof(meshx_provision_capabilites_t) +
                              sizeof(meshx_provision_start_t) + 128;

    uint8_t *pcfm_inputs = meshx_malloc(cfm_inputs_len);
    if (NULL == pcfm_inputs)
    {
        MESHX_ERROR("out of memory!");
        return -MESHX_ERR_MEM;
    }

    uint8_t *pdata = pcfm_inputs;

    memcpy(pdata, &prov_dev->invite, sizeof(meshx_provision_invite_t));
    pdata += sizeof(meshx_provision_invite_t);
    memcpy(pdata, &prov_dev->capabilites, sizeof(meshx_provision_capabilites_t));
    pdata += sizeof(meshx_provision_capabilites_t);
    memcpy(pdata, &prov_dev->start, sizeof(meshx_provision_start_t));
    pdata += sizeof(meshx_provision_start_t);
    if (MESHX_ROLE_DEVICE == prov_dev->role)
    {
        memcpy(pdata, prov_dev->public_key_remote, 64);
        pdata += 64;
        memcpy(pdata, prov_dev->public_key, 64);
    }
    else
    {
        memcpy(pdata, prov_dev->public_key, 64);
        pdata += 64;
        memcpy(pdata, prov_dev->public_key_remote, 64);
    }
    uint8_t cfm_salt[16];
    meshx_s1(pcfm_inputs, cfm_inputs_len, cfm_salt);
    uint8_t cfm_key[16];
    uint8_t N[] = {'p', 'r', 'c', 'k'};
    meshx_k1(prov_dev->share_secret, 32, cfm_salt, N, sizeof(N), cfm_key);

    uint32_t *prandom = (uint32_t *)prov_dev->random;
    for (uint8_t i = 0; i < 4; ++i)
    {
        *prandom = MESHX_ABS(meshx_rand());
    }
    uint8_t cfm_data[32];
    memcpy(cfm_data, prov_dev->random, 16);
    memcpy(cfm_data + 16, auth_value, 16);
    meshx_aes_cmac(cfm_key, cfm_data, sizeof(cfm_data), prov_dev->confirmation);

    return MESHX_SUCCESS;
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
    prov_dev->invite = invite;
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

    if ((prov_dev->state < MESHX_PROVISION_STATE_INVITE) ||
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
    prov_dev->capabilites = *pcap;
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

    if ((prov_dev->state < MESHX_PROVISION_STATE_CAPABILITES) ||
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
    prov_dev->start = *pstart;
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

int32_t meshx_provision_public_key(meshx_provision_dev_t prov_dev,
                                   const meshx_provision_public_key_t *ppub_key)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("provision device value is NULL");
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_START) ||
        (prov_dev->state > MESHX_PROVISION_STATE_PUBLIC_KEY))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    int32_t ret = MESHX_SUCCESS;
    memcpy(prov_dev->public_key, ppub_key, 64);
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_public_key(prov_dev, ppub_key);
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

int32_t meshx_provision_failed(meshx_provision_dev_t prov_dev, uint8_t err_code)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("provision device value is NULL");
        return -MESHX_ERR_INVAL;
    }

    if (prov_dev->state < MESHX_PROVISION_STATE_INVITE)
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    if ((MESHX_PROVISION_STATE_COMPLETE == prov_dev->state) ||
        (MESHX_PROVISION_STATE_FAILED == prov_dev->state))
    {
        MESHX_WARN("already in provision finishing procedure: %d", prov_dev->state);
        return -MESHX_ERR_ALREADY;
    }

    int32_t ret = MESHX_SUCCESS;
    prov_dev->err_code = err_code;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_failed(prov_dev, err_code);
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
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_LENGTH;
        }
        else if (prov_dev->state > MESHX_PROVISION_STATE_INVITE)
        {
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            prov_dev->state = MESHX_PROVISION_STATE_INVITE;
            prov_dev->invite = pprov_pdu->invite;
            /* notify app invite value */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_INVITE;
            notify_prov.pdata = &pprov_pdu->invite;
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
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_LENGTH;
        }
        else if (prov_dev->state > MESHX_PROVISION_STATE_CAPABILITES)
        {
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            prov_dev->state = MESHX_PROVISION_STATE_CAPABILITES;
            prov_dev->capabilites = pprov_pdu->capabilites;
            /* notify app capabilites value */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_CAPABILITES;
            notify_prov.pdata = &pprov_pdu->capabilites;
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
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_LENGTH;
        }
        else if (prov_dev->state > MESHX_PROVISION_STATE_START)
        {
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            prov_dev->state = MESHX_PROVISION_STATE_START;
            prov_dev->start = pprov_pdu->start;
            /* notify app start value */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_START;
            notify_prov.pdata = &pprov_pdu->start;
            meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                         sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_start_t));
        }
        break;
#endif
    case MESHX_PROVISION_TYPE_PUBLIC_KEY:
        if (len < sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_public_key_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid public key pdu length: %d", len);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_LENGTH;
        }
        else if (prov_dev->state > MESHX_PROVISION_STATE_PUBLIC_KEY)
        {
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            if (meshx_provision_validate_public_key(&pprov_pdu->public_key))
            {
                prov_dev->state = MESHX_PROVISION_STATE_PUBLIC_KEY;
                memcpy(prov_dev->public_key_remote, &pprov_pdu->public_key, 64);
                /* notify app public key value */
                meshx_notify_prov_t notify_prov;
                notify_prov.metadata.prov_dev = prov_dev;
                notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_PUBLIC_KEY;
                notify_prov.pdata = &pprov_pdu->public_key;
                meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                             sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_public_key_t));

                /* generate secret */
                meshx_ecc_shared_secret((const uint8_t *)&pprov_pdu->public_key, prov_dev->private_key,
                                        prov_dev->share_secret);
                MESHX_INFO("shared secret:");
                MESHX_DUMP_INFO(prov_dev->share_secret, 32);
            }
            else
            {
                MESHX_ERROR("invalid public key!");
                /* send provision failed */
                meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
                ret = -MESHX_ERR_INVAL;
            }

        }
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
#if MESHX_SUPPORT_ROLE_PROVISIONER
    case MESHX_PROVISION_TYPE_COMPLETE:
        break;
    case MESHX_PROVISION_TYPE_FAILED:
        if (len < sizeof(meshx_provision_pdu_metadata_t) + sizeof(uint8_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid provision failed pdu length: %d", len);
            ret = -MESHX_ERR_LENGTH;
        }
        else
        {
            prov_dev->state = MESHX_PROVISION_STATE_FAILED;
            /* notify app provision failed error code */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_FAILED;
            notify_prov.pdata = &pprov_pdu->err_code;
            meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                         sizeof(meshx_notify_prov_metadata_t) + sizeof(uint8_t));
        }
        break;
#endif
    default:
        /* send provision failed */
        meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_PDU);
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
        {
            const meshx_provision_link_open_result_t *presult = pnotify->pdata;
            if (MESHX_PROVISION_LINK_OPEN_SUCCESS == *presult)
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
    case MESHX_PROV_NOTIFY_TRANS_ACK:
        {
            const meshx_provision_state_t *pstate = pnotify->pdata;
            if ((MESHX_PROVISION_STATE_FAILED == *pstate) ||
                (MESHX_PROVISION_STATE_COMPLETE == *pstate))
            {
                switch (bearer->type)
                {
                case MESHX_BEARER_TYPE_ADV:
                    {
                        uint8_t reason = MESHX_PROVISION_LINK_CLOSE_FAIL;
                        /* send link close */
                        if (MESHX_PROVISION_STATE_COMPLETE == *pstate)
                        {
                            reason = MESHX_PROVISION_LINK_CLOSE_SUCCESS;
                        }
                        meshx_pb_adv_link_close(pnotify->metadata.prov_dev, reason);
                    }
                    break;
                case MESHX_BEARER_TYPE_GATT:
                    break;
                default:
                    MESHX_ERROR("invalid bearer type: %d", bearer->type);
                    break;
                }
            }
        }
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
