/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_PROVISION"
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
#include "meshx_endianness.h"

#define MESHX_SAMPLE_DATA     1

#if MESHX_SAMPLE_DATA
const uint8_t prov_public_key[64] =
{
    0x2c, 0x31, 0xa4, 0x7b, 0x57, 0x79, 0x80, 0x9e,
    0xf4, 0x4c, 0xb5, 0xea, 0xaf, 0x5c, 0x3e, 0x43,
    0xd5, 0xf8, 0xfa, 0xad, 0x4a, 0x87, 0x94, 0xcb,
    0x98, 0x7e, 0x9b, 0x03, 0x74, 0x5c, 0x78, 0xdd,
    0x91, 0x95, 0x12, 0x18, 0x38, 0x98, 0xdf, 0xbe,
    0xcd, 0x52, 0xe2, 0x40, 0x8e, 0x43, 0x87, 0x1f,
    0xd0, 0x21, 0x10, 0x91, 0x17, 0xbd, 0x3e, 0xd4,
    0xea, 0xf8, 0x43, 0x77, 0x43, 0x71, 0x5d, 0x4f
};

const uint8_t prov_private_key[32] =
{
    0x06, 0xa5, 0x16, 0x69, 0x3c, 0x9a, 0xa3, 0x1a,
    0x60, 0x84, 0x54, 0x5d, 0x0c, 0x5d, 0xb6, 0x41,
    0xb4, 0x85, 0x72, 0xb9, 0x72, 0x03, 0xdd, 0xff,
    0xb7, 0xac, 0x73, 0xf7, 0xd0, 0x45, 0x76, 0x63
};

const uint8_t prov_random[16] =
{
    0x8b, 0x19, 0xac, 0x31, 0xd5, 0x8b, 0x12, 0x4c,
    0x94, 0x62, 0x09, 0xb5, 0xdb, 0x10, 0x21, 0xb9
};

const uint8_t device_public_key[64] =
{
    0xf4, 0x65, 0xe4, 0x3f, 0xf2, 0x3d, 0x3f, 0x1b,
    0x9d, 0xc7, 0xdf, 0xc0, 0x4d, 0xa8, 0x75, 0x81,
    0x84, 0xdb, 0xc9, 0x66, 0x20, 0x47, 0x96, 0xec,
    0xcf, 0x0d, 0x6c, 0xf5, 0xe1, 0x65, 0x00, 0xcc,
    0x02, 0x01, 0xd0, 0x48, 0xbc, 0xbb, 0xd8, 0x99,
    0xee, 0xef, 0xc4, 0x24, 0x16, 0x4e, 0x33, 0xc2,
    0x01, 0xc2, 0xb0, 0x10, 0xca, 0x6b, 0x4d, 0x43,
    0xa8, 0xa1, 0x55, 0xca, 0xd8, 0xec, 0xb2, 0x79
};

const uint8_t device_private_key[32] =
{
    0x52, 0x9a, 0xa0, 0x67, 0x0d, 0x72, 0xcd, 0x64,
    0x97, 0x50, 0x2e, 0xd4, 0x73, 0x50, 0x2b, 0x03,
    0x7e, 0x88, 0x03, 0xb5, 0xc6, 0x08, 0x29, 0xa5,
    0xa3, 0xca, 0xa2, 0x19, 0x50, 0x55, 0x30, 0xba
};

const uint8_t device_random[16] =
{
    0x55, 0xa2, 0xa2, 0xbc, 0xa0, 0x4c, 0xd3, 0x2f,
    0xf6, 0xf3, 0x46, 0xbd, 0x0a, 0x0c, 0x1a, 0x3a
};

static meshx_provision_capabilites_t prov_capabilites =
{
    .element_nums = 1,
    .algorithms = 1,
    .public_key_type = 0,
    .static_oob_type = 0,
    .output_oob_size = 0,
    .output_oob_action = 0,
    .input_oob_size = 0,
    .input_oob_action = 0,
};

static meshx_provision_start_t prov_start =
{
    .algorithm = 0,
    .public_key = 0,
    .auth_method = 0,
    .auth_action = 0,
    .auth_size = 0,
};

static meshx_provision_data_t prov_data =
{
    .network_key = {0xef, 0xb2, 0x25, 0x5e, 0x64, 0x22, 0xd3, 0x30, 0x08, 0x8e, 0x09, 0xbb, 0x01, 0x5e, 0xd7, 0x07},
    .key_index = 0x0567,
    .key_refresh_flag = 0,
    .iv_update_flag = 0,
    .rfu = 0,
    .iv_index = 0x01020304,
    .unicast_address = 0x0b0c,
    .mic = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
#endif

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

    int32_t ret = MESHX_SUCCESS;

#if MESHX_SAMPLE_DATA
    if (MESHX_ROLE_DEVICE == prov_dev->role)
    {
        memcpy(prov_dev->public_key, device_public_key, sizeof(prov_dev->public_key));
        memcpy(prov_dev->private_key, device_private_key, sizeof(prov_dev->private_key));
    }
    else
    {
        memcpy(prov_dev->public_key, prov_public_key, sizeof(prov_dev->public_key));
        memcpy(prov_dev->private_key, prov_private_key, sizeof(prov_dev->private_key));
    }
#else
    ret = meshx_ecc_make_key(prov_dev->public_key, prov_dev->private_key);
#endif
    MESHX_DEBUG("public key:");
    MESHX_DUMP_DEBUG(prov_dev->public_key, sizeof(prov_dev->public_key));
    MESHX_DEBUG("private key:");
    MESHX_DUMP_DEBUG(prov_dev->private_key, sizeof(prov_dev->private_key));

    return ret;
}

int32_t meshx_provision_get_local_public_key(meshx_provision_dev_t prov_dev,
                                             meshx_provision_public_key_t *pkey)
{
    if ((NULL == prov_dev) || (NULL == pkey))
    {
        MESHX_ERROR("invalid parameter: prov_dev 0x%x, pkey 0x%x");
        return -MESHX_ERR_INVAL;
    }

    memcpy(pkey, prov_dev->public_key, sizeof(meshx_provision_public_key_t));

    return MESHX_SUCCESS;
}

bool meshx_provision_validate_public_key(const meshx_provision_public_key_t *pkey)
{
    return meshx_ecc_validate_public_key((const uint8_t *)pkey);
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

int32_t meshx_provision_generate_auth_value(meshx_provision_dev_t prov_dev,
                                            const uint8_t *pauth_value, uint8_t len)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("invalid provision device!");
        return -MESHX_ERR_INVAL;
    }

    int32_t ret = MESHX_SUCCESS;
    switch (prov_dev->start.auth_method)
    {
    case MESHX_PROVISION_AUTH_METHOD_NO_OOB:
        memset(&prov_dev->auth_value, 0, sizeof(meshx_provision_auth_value_t));
        break;
    case MESHX_PROVISION_AUTH_METHOD_STATIC_OOB:
        if ((len >= sizeof(meshx_provision_auth_value_t)) && (NULL != pauth_value))
        {
            memcpy(&prov_dev->auth_value, pauth_value, sizeof(meshx_provision_auth_value_t));
        }
        else
        {
            MESHX_ERROR("invalid value or length: 0x%x, %d", pauth_value, len);
            ret = -MESHX_ERR_LENGTH;
        }
        break;
    case MESHX_PROVISION_AUTH_METHOD_OUTPUT_OOB:
    case MESHX_PROVISION_AUTH_METHOD_INPUT_OOB:
        if (NULL != pauth_value)
        {
            if ((MESHX_PROVISION_AUTH_ACTION_BLINK == prov_dev->start.auth_action) ||
                (MESHX_PROVISION_AUTH_ACTION_BEEP == prov_dev->start.auth_action) ||
                (MESHX_PROVISION_AUTH_ACTION_VIBRATE == prov_dev->start.auth_action) ||
                (MESHX_PROVISION_AUTH_ACTION_OUT_NUMERIC == prov_dev->start.auth_action))
            {
                uint32_t value = *((const uint32_t *)pauth_value);
                for (int8_t i = 15; i > 0; i--)
                {
                    if (0 != value)
                    {
                        prov_dev->auth_value.auth_value[i] = (value & 0xff);
                        value >>= 8;
                    }
                    else
                    {
                        prov_dev->auth_value.auth_value[i] = 0x00;
                    }
                }
            }
            else
            {
                for (uint8_t i = 0; i < 16; ++i)
                {
                    if (len > 0)
                    {
                        prov_dev->auth_value.auth_value[i] = pauth_value[i];
                        len --;
                    }
                    else
                    {
                        prov_dev->auth_value.auth_value[i] = 0x00;
                    }

                }
            }
        }
        else
        {
            MESHX_ERROR("auth value is NULL");
            ret = -MESHX_ERR_INVAL;
        }
        break;
    default:
        break;
    }
    return ret;
}

int32_t meshx_provision_generate_random(meshx_provision_dev_t prov_dev)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("invalid provision device!");
        return -MESHX_ERR_INVAL;
    }

#if MESHX_SAMPLE_DATA
    if (MESHX_ROLE_DEVICE == prov_dev->role)
    {
        memcpy(&prov_dev->random, device_random, sizeof(device_random));
    }
    else
    {
        memcpy(&prov_dev->random, prov_random, sizeof(prov_random));
    }
#else
    uint32_t *prandom = (uint32_t *)&prov_dev->random;
    for (uint8_t i = 0; i < 4; ++i)
    {
        *prandom ++ = MESHX_ABS(meshx_rand());
    }
#endif

    MESHX_DEBUG("random:");
    MESHX_DUMP_DEBUG(&prov_dev->random, sizeof(prov_dev->random));

    return MESHX_SUCCESS;
}

int32_t meshx_provision_get_random(meshx_provision_dev_t prov_dev,
                                   meshx_provision_random_t *prandom)
{
    if ((NULL == prov_dev)  || (NULL == prandom))
    {
        MESHX_ERROR("invlaid parameter: prov_dev 0x%x, prandom 0x%x", prov_dev, prandom);
        return -MESHX_ERR_INVAL;
    }

    memcpy(prandom, &prov_dev->random, sizeof(meshx_provision_random_t));

    return MESHX_SUCCESS;
}

int32_t meshx_provision_generate_confirmation(meshx_provision_dev_t prov_dev,
                                              const meshx_provision_random_t *prandom)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("invalid provision device!");
        return -MESHX_ERR_INVAL;
    }

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
    meshx_s1(pcfm_inputs, cfm_inputs_len, prov_dev->confirmation_salt);
    meshx_free(pcfm_inputs);
    MESHX_DEBUG("confirmation salt:");
    MESHX_DUMP_DEBUG(prov_dev->confirmation_salt, 16);

    uint8_t cfm_key[16];
    uint8_t P[] = {'p', 'r', 'c', 'k'};
    meshx_k1(prov_dev->share_secret, 32, prov_dev->confirmation_salt, P, sizeof(P), cfm_key);
    MESHX_DEBUG("confirmation key:");
    MESHX_DUMP_DEBUG(cfm_key, 16);

    uint8_t cfm_data[sizeof(meshx_provision_random_t) + sizeof(meshx_provision_auth_value_t)];
    memcpy(cfm_data, prandom, sizeof(meshx_provision_random_t));

    memcpy(cfm_data + sizeof(meshx_provision_random_t), &prov_dev->auth_value,
           sizeof(meshx_provision_auth_value_t));
    meshx_aes_cmac(cfm_key, cfm_data, sizeof(cfm_data), (uint8_t *)&prov_dev->confirmation);
    MESHX_DEBUG("confirmation:");
    MESHX_DUMP_DEBUG(&prov_dev->confirmation, sizeof(meshx_provision_confirmation_t));

    return MESHX_SUCCESS;
}

int32_t meshx_provision_get_confirmation(meshx_provision_dev_t prov_dev,
                                         meshx_provision_confirmation_t *pcfm)
{
    if ((NULL == prov_dev)  || (NULL == pcfm))
    {
        MESHX_ERROR("invlaid parameter: prov_dev 0x%x, pcfm 0x%x", prov_dev, pcfm);
        return -MESHX_ERR_INVAL;
    }

    memcpy(pcfm, &prov_dev->confirmation, sizeof(meshx_provision_confirmation_t));

    return MESHX_SUCCESS;
}

static bool meshx_provision_verify_confirmation(meshx_provision_dev_t prov_dev)
{
    meshx_provision_generate_confirmation(prov_dev, &prov_dev->random_remote);
    return (0 == memcmp(&prov_dev->confirmation, &prov_dev->confirmation_remote,
                        sizeof(meshx_provision_confirmation_t)));
}

static int32_t meshx_provision_calculate_session_key(meshx_provision_dev_t prov_dev)
{
    uint8_t prov_salt_inputs[sizeof(prov_dev->confirmation_salt) + sizeof(
                                                                     meshx_provision_random_t) * 2];
    memcpy(prov_salt_inputs, prov_dev->confirmation_salt, sizeof(prov_dev->confirmation_salt));
    if (MESHX_ROLE_DEVICE == prov_dev->role)
    {
        memcpy(prov_salt_inputs + sizeof(meshx_provision_random_t), &prov_dev->random_remote,
               sizeof(meshx_provision_random_t));
        memcpy(prov_salt_inputs + sizeof(meshx_provision_random_t) * 2, &prov_dev->random,
               sizeof(meshx_provision_random_t));
    }
    else
    {
        memcpy(prov_salt_inputs + sizeof(meshx_provision_random_t), &prov_dev->random,
               sizeof(meshx_provision_random_t));
        memcpy(prov_salt_inputs + sizeof(meshx_provision_random_t) * 2, &prov_dev->random_remote,
               sizeof(meshx_provision_random_t));
    }

    uint8_t prov_salt[16];
    meshx_s1(prov_salt_inputs, sizeof(prov_salt_inputs), prov_salt);
    MESHX_DEBUG("provision salt:");
    MESHX_DUMP_DEBUG(prov_salt, 16);

    uint8_t P[] = {'p', 'r', 's', 'k'};
    meshx_k1(prov_dev->share_secret, sizeof(prov_dev->share_secret), prov_salt, P, sizeof(P),
             prov_dev->session_key);
    MESHX_DEBUG("session key:");
    MESHX_DUMP_DEBUG(prov_dev->session_key, 16);

    P[3] = 'n';
    meshx_k1(prov_dev->share_secret, sizeof(prov_dev->share_secret), prov_salt, P, sizeof(P),
             prov_dev->session_nonce);
    MESHX_DEBUG("session nonce:");
    MESHX_DUMP_DEBUG(prov_dev->session_nonce, 16);

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

    prov_dev->state = MESHX_PROVISION_STATE_INVITE;
    int32_t ret = MESHX_SUCCESS;
    prov_dev->invite = invite;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_invite(prov_dev);
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
    if ((NULL == prov_dev) || (NULL == pcap))
    {
        MESHX_ERROR("invalid value: prov_dev 0x%x, pcap 0x%x", prov_dev, pcap);
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

    prov_dev->state = MESHX_PROVISION_STATE_CAPABILITES;
    int32_t ret = MESHX_SUCCESS;
#if MESHX_SAMPLE_DATA
    prov_dev->capabilites = prov_capabilites;
#else
    prov_dev->capabilites = *pcap;
#endif

    /* convert endianness */
    prov_dev->capabilites.algorithms = MESHX_HOST_TO_BE16(prov_dev->capabilites.algorithms);
    prov_dev->capabilites.output_oob_action = MESHX_HOST_TO_BE16(
                                                  prov_dev->capabilites.output_oob_action);
    prov_dev->capabilites.input_oob_action = MESHX_HOST_TO_BE16(prov_dev->capabilites.input_oob_action);

    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_capabilites(prov_dev);
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
    if ((NULL == prov_dev) || (NULL == pstart))
    {
        MESHX_ERROR("invalid value: prov_dev 0x%x, pstart 0x%x", prov_dev, pstart);
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

    prov_dev->state = MESHX_PROVISION_STATE_START;
    int32_t ret = MESHX_SUCCESS;
#if MESHX_SAMPLE_DATA
    prov_dev->start = prov_start;
#else
    prov_dev->start = *pstart;
#endif
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_start(prov_dev);
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
    if ((NULL == prov_dev) || (NULL == ppub_key))
    {
        MESHX_ERROR("invalid value: prov_dev 0x%x, ppub_key 0x%x", prov_dev, ppub_key);
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_START) ||
        (prov_dev->state > MESHX_PROVISION_STATE_PUBLIC_KEY))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    /*
    // both side send public key, when receive public key, will set local state to public key
    // is do this judgement, public can not send
    // TODO: need to use to state? MESHX_PROVISION_STATE_PUBLIC_KEY, MESHX_PROVISION_STATE_PUBLIC_KEY_REMOTE?
    if (MESHX_PROVISION_STATE_PUBLIC_KEY == prov_dev->state)
    {
        MESHX_WARN("already in public key procedure");
        return -MESHX_ERR_ALREADY;
    }
    */

    prov_dev->state = MESHX_PROVISION_STATE_PUBLIC_KEY;
    int32_t ret = MESHX_SUCCESS;
    memcpy(prov_dev->public_key, ppub_key, sizeof(meshx_provision_public_key_t));
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_public_key(prov_dev);
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

int32_t meshx_provision_confirmation(meshx_provision_dev_t prov_dev,
                                     const meshx_provision_confirmation_t *pcfm)
{
    if ((NULL == prov_dev) || (NULL == pcfm))
    {
        MESHX_ERROR("invalid value: prov_dev 0x%x, pcfm 0x%x", prov_dev, pcfm);
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_PUBLIC_KEY) ||
        (prov_dev->state > MESHX_PROVISION_STATE_CONFIRMATION))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    prov_dev->state = MESHX_PROVISION_STATE_CONFIRMATION;
    int32_t ret = MESHX_SUCCESS;
    prov_dev->confirmation = *pcfm;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_confirmation(prov_dev);
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

int32_t meshx_provision_random(meshx_provision_dev_t prov_dev,
                               const meshx_provision_random_t *prandom)
{
    if ((NULL == prov_dev) || (NULL == prandom))
    {
        MESHX_ERROR("invalid value: prov_dev 0x%x, prandom 0x%x", prov_dev, prandom);
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_CONFIRMATION) ||
        (prov_dev->state > MESHX_PROVISION_STATE_RANDOM))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    prov_dev->state = MESHX_PROVISION_STATE_RANDOM;
    int32_t ret = MESHX_SUCCESS;
    prov_dev->random = *prandom;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_random(prov_dev);
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

int32_t meshx_provision_data(meshx_provision_dev_t prov_dev,
                             const meshx_provision_data_t *pdata)
{
    if ((NULL == prov_dev) || (NULL == pdata))
    {
        MESHX_ERROR("invalid value: prov_dev 0x%x, prandom 0x%x", prov_dev, pdata);
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_RANDOM) ||
        (prov_dev->state > MESHX_PROVISION_STATE_DATA))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    if (MESHX_PROVISION_STATE_DATA == prov_dev->state)
    {
        MESHX_WARN("already in data procedure");
        return -MESHX_ERR_ALREADY;
    }

    prov_dev->state = MESHX_PROVISION_STATE_DATA;
    int32_t ret = MESHX_SUCCESS;
#if MESHX_SAMPLE_DATA
    prov_dev->data = prov_data;
#else
    prov_dev->data = *pdata;
#endif
    prov_dev->data.key_index = MESHX_HOST_TO_BE16(prov_dev->data.key_index);
    prov_dev->data.iv_index = MESHX_HOST_TO_BE32(prov_dev->data.iv_index);
    prov_dev->data.unicast_address = MESHX_HOST_TO_BE16(prov_dev->data.unicast_address);

    MESHX_DEBUG("provision data:");
    MESHX_DUMP_DEBUG(&prov_dev->data, sizeof(prov_dev->data) - sizeof(prov_dev->data.mic));
    /* encrypt data */
    meshx_aes_ccm_encrypt(prov_dev->session_key, prov_dev->session_nonce + 3, 13, NULL, 0,
                          (const uint8_t *)&prov_dev->data, sizeof(prov_dev->data) - sizeof(prov_dev->data.mic),
                          (uint8_t *)&prov_dev->data, prov_dev->data.mic, 8);//sizeof(prov_dev->data.mic));

    MESHX_DEBUG("encrypt data:");
    MESHX_DUMP_DEBUG(&prov_dev->data, sizeof(prov_dev->data) - sizeof(prov_dev->data.mic));
    MESHX_DEBUG("data mic:");
    MESHX_DUMP_DEBUG(&prov_dev->data.mic, sizeof(prov_dev->data.mic));

    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_data(prov_dev);
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

int32_t meshx_provision_complete(meshx_provision_dev_t prov_dev)
{
    if (NULL == prov_dev)
    {
        MESHX_ERROR("provision device value is NULL");
        return -MESHX_ERR_INVAL;
    }

    if ((prov_dev->state < MESHX_PROVISION_STATE_DATA) ||
        (prov_dev->state > MESHX_PROVISION_STATE_COMPLETE))
    {
        MESHX_ERROR("invalid state: %d", prov_dev->state);
        return -MESHX_ERR_STATE;
    }

    if (MESHX_PROVISION_STATE_COMPLETE == prov_dev->state)
    {
        MESHX_WARN("already in complete procedure");
        return -MESHX_ERR_ALREADY;
    }

    prov_dev->state = MESHX_PROVISION_STATE_COMPLETE;
    int32_t ret = MESHX_SUCCESS;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_complete(prov_dev);
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

    prov_dev->state = MESHX_PROVISION_STATE_FAILED;
    int32_t ret = MESHX_SUCCESS;
    prov_dev->err_code = err_code;
    switch (prov_dev->bearer->type)
    {
    case MESHX_BEARER_TYPE_ADV:
        ret = meshx_pb_adv_failed(prov_dev);
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

static bool meshx_provision_validate_start(meshx_provision_dev_t prov_dev,
                                           const meshx_provision_start_t *pstart)
{
    /* validate value range */
    if (pstart->algorithm >= MESHX_PROVISION_CAP_ALGORITHM_P256_CURVE)
    {
        return FALSE;
    }

    if (pstart->public_key >= MESHX_PROVISION_CAP_PUBLIC_KEY_RFU)
    {
        return FALSE;
    }

    if (pstart->auth_method >= MESHX_PROVISION_AUTH_METHOD_PROHIBITED)
    {
        return FALSE;
    }

    if (pstart->auth_action >= MESHX_PROVISION_AUTH_ACTION_RFU)
    {
        return FALSE;
    }

    if ((pstart->auth_method == MESHX_PROVISION_AUTH_METHOD_OUTPUT_OOB) ||
        (pstart->auth_method == MESHX_PROVISION_AUTH_METHOD_INPUT_OOB))
    {
        if ((pstart->auth_size < MESHX_PROVISION_AUTH_SIZE_MIN) ||
            (pstart->auth_size > MESHX_PROVISION_AUTH_SIZE_MAX))
        {
            return FALSE;
        }
    }

    return TRUE;
}

#if MESHX_SUPPORT_ROLE_DEVICE
static bool meshx_provision_is_start_supported(meshx_provision_dev_t prov_dev,
                                               const meshx_provision_start_t *pstart)
{
    /* validate value and capabilites */
    if (pstart->public_key != prov_dev->capabilites.public_key_type)
    {
        return FALSE;
    }

    if (MESHX_PROVISION_AUTH_METHOD_STATIC_OOB == pstart->auth_method)
    {
        if (MESHX_PROVISION_CAP_STATIC_OOB_NOT_AVAIABLE == prov_dev->capabilites.static_oob_type)
        {
            return FALSE;
        }
    }
    else if (MESHX_PROVISION_AUTH_METHOD_OUTPUT_OOB == pstart->auth_method)
    {
        if (MESHX_PROVISION_CAP_NOT_SUPPORT_OUTPUT_OOB == prov_dev->capabilites.output_oob_size)
        {
            return FALSE;
        }
        else if (pstart->auth_size > prov_dev->capabilites.output_oob_size)
        {
            return FALSE;
        }
        else
        {
            uint16_t auth_action = pstart->auth_action;
            if (0 != auth_action)
            {
                auth_action = (1 << (auth_action - 1));
            }

            if (0 == (auth_action & prov_dev->capabilites.output_oob_action))
            {
                return FALSE;
            }
        }
    }
    else if (MESHX_PROVISION_AUTH_METHOD_INPUT_OOB == pstart->auth_method)
    {
        if (MESHX_PROVISION_CAP_NOT_SUPPORT_INPUT_OOB == prov_dev->capabilites.output_oob_size)
        {
            return FALSE;
        }
        else if (pstart->auth_size > prov_dev->capabilites.input_oob_size)
        {
            return FALSE;
        }
        else
        {
            uint16_t auth_action = pstart->auth_action;
            if (0 != auth_action)
            {
                auth_action = (1 << (auth_action - 1));
            }

            if (0 == (auth_action & prov_dev->capabilites.input_oob_action))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}
#endif

int32_t meshx_provision_pdu_process(meshx_provision_dev_t prov_dev,
                                    const uint8_t *pdata, uint8_t len)
{
    const meshx_provision_pdu_t *pprov_pdu = (const meshx_provision_pdu_t *)pdata;
    int32_t ret = MESHX_SUCCESS;
    switch (pprov_pdu->metadata.type)
    {
#if MESHX_SUPPORT_ROLE_DEVICE
    case MESHX_PROVISION_TYPE_INVITE:
        MESHX_DEBUG("processing invite");
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
            MESHX_ERROR("invalid state in processing invire: %d", prov_dev->state);
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
        MESHX_DEBUG("processing capabilites");
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
            MESHX_ERROR("invalid state in processing capabilites: %d", prov_dev->state);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            prov_dev->state = MESHX_PROVISION_STATE_CAPABILITES;
            prov_dev->capabilites = pprov_pdu->capabilites;

#if 0
            /* convert endianness */
            prov_dev->capabilites.algorithms = MESHX_BE16_TO_HOST(prov_dev->capabilites.algorithms);
            prov_dev->capabilites.output_oob_action = MESHX_BE16_TO_HOST(
                                                          prov_dev->capabilites.output_oob_action);
            prov_dev->capabilites.input_oob_action = MESHX_BE16_TO_HOST(prov_dev->capabilites.input_oob_action);
#endif

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
        MESHX_DEBUG("processing start");
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
            MESHX_ERROR("invalid state in processing start: %d", prov_dev->state);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else if (!meshx_provision_validate_start(prov_dev, &pprov_pdu->start))
        {
            MESHX_ERROR("invalid start value");
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_INVAL;
        }
        else if (!meshx_provision_is_start_supported(prov_dev, &pprov_pdu->start))
        {
            MESHX_ERROR("unsupported start value");
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_OUT_OF_RESOURCE);
            ret = -MESHX_ERR_RESOURCE;
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
        MESHX_DEBUG("processing public key");
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
            MESHX_ERROR("invalid state in processing public key: %d", prov_dev->state);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            if (meshx_provision_validate_public_key(&pprov_pdu->public_key))
            {
                /* generate secret */
                meshx_ecc_shared_secret((const uint8_t *)&pprov_pdu->public_key, prov_dev->private_key,
                                        prov_dev->share_secret);
                MESHX_DEBUG("shared secret:");
                MESHX_DUMP_DEBUG(prov_dev->share_secret, 32);

                /* notify app public key */
                prov_dev->state = MESHX_PROVISION_STATE_PUBLIC_KEY;
                memcpy(prov_dev->public_key_remote, &pprov_pdu->public_key, 64);
                /* notify app public key value */
                meshx_notify_prov_t notify_prov;
                notify_prov.metadata.prov_dev = prov_dev;
                notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_PUBLIC_KEY;
                notify_prov.pdata = &pprov_pdu->public_key;
                meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                             sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_public_key_t));
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
        MESHX_DEBUG("processing input complete");
        break;
    case MESHX_PROVISION_TYPE_CONFIRMATION:
        MESHX_DEBUG("processing confirmation");
        if (len < sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_confirmation_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid confirmaiton pdu length: %d", len);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_LENGTH;
        }
        else if (prov_dev->state > MESHX_PROVISION_STATE_CONFIRMATION)
        {
            MESHX_ERROR("invalid state in processing confirmation: %d", prov_dev->state);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            prov_dev->state = MESHX_PROVISION_STATE_CONFIRMATION;
            prov_dev->confirmation_remote = pprov_pdu->confirmation;
            /* notify app confirmation vlaue */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_CONFIRMATION;
            notify_prov.pdata = &pprov_pdu->confirmation;
            meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                         sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_confirmation_t));
        }
        break;
    case MESHX_PROVISION_TYPE_RANDOM:
        MESHX_DEBUG("processing random");
        if (len < sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_confirmation_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid confirmaiton pdu length: %d", len);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_LENGTH;
        }
        else if (prov_dev->state > MESHX_PROVISION_STATE_RANDOM)
        {
            MESHX_ERROR("invalid state in processing random: %d", prov_dev->state);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            prov_dev->random_remote = pprov_pdu->random;
            if (meshx_provision_verify_confirmation(prov_dev))
            {
                /* generate session key */
                meshx_provision_calculate_session_key(prov_dev);

                prov_dev->state = MESHX_PROVISION_STATE_RANDOM;
                /* notify app random value */
                meshx_notify_prov_t notify_prov;
                notify_prov.metadata.prov_dev = prov_dev;
                notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_RANDOM;
                notify_prov.pdata = &pprov_pdu->random;
                meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                             sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_random_t));
            }
            else
            {
                MESHX_ERROR("confirmation verified failed!");
                /* send provision failed */
                meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_CONFIMATION_FAILED);
                ret = -MESHX_ERR_FAIL;
            }
        }
        break;
#if MESHX_SUPPORT_ROLE_DEVICE
    case MESHX_PROVISION_TYPE_DATA:
        MESHX_DEBUG("processing data");
        if (len < sizeof(meshx_provision_pdu_metadata_t) + sizeof(meshx_provision_data_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid data pdu length: %d", len);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_LENGTH;
        }
        else if (prov_dev->state > MESHX_PROVISION_STATE_DATA)
        {
            MESHX_ERROR("invalid state in processing data: %d", prov_dev->state);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            /* decrypt data */
            if (MESHX_SUCCESS == meshx_aes_ccm_decrypt(prov_dev->session_key, prov_dev->session_nonce + 3, 13,
                                                       NULL,
                                                       0,
                                                       (const uint8_t *)&pprov_pdu->data, sizeof(prov_dev->data) - sizeof(prov_dev->data.mic),
                                                       (uint8_t *)&prov_dev->data, pprov_pdu->data.mic, 8))//sizeof(prov_dev->data.mic)))
            {

                prov_dev->state = MESHX_PROVISION_STATE_DATA;

#if 0
                /* convert endianness */
                prov_dev->data.key_index = MESHX_BE16_TO_HOST(prov_dev->data.key_index);
                prov_dev->data.iv_index = MESHX_BE32_TO_HOST(prov_dev->data.iv_index);
                prov_dev->data.unicast_address = MESHX_BE16_TO_HOST(prov_dev->data.unicast_address);
#endif
                /* notify app data value */
                meshx_notify_prov_t notify_prov;
                notify_prov.metadata.prov_dev = prov_dev;
                notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_DATA;
                notify_prov.pdata = &prov_dev->data;
                meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                             sizeof(meshx_notify_prov_metadata_t) + sizeof(meshx_provision_data_t));
            }
            else
            {
                MESHX_ERROR("decrypt provision data failed!");
                /* send provision failed */
                meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_DECRYPTION_FAILED);
                ret = -MESHX_ERR_STATE;
            }
        }
        break;
#endif
#if MESHX_SUPPORT_ROLE_PROVISIONER
    case MESHX_PROVISION_TYPE_COMPLETE:
        MESHX_DEBUG("processing complete");
        if (len < sizeof(meshx_provision_pdu_metadata_t))
        {
            /* provision failed: invalid format */
            MESHX_ERROR("invalid complete pdu length: %d", len);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_INVALID_FORMAT);
            ret = -MESHX_ERR_LENGTH;
        }
        else if (prov_dev->state > MESHX_PROVISION_STATE_COMPLETE)
        {
            MESHX_ERROR("invalid state in processing complete: %d", prov_dev->state);
            /* send provision failed */
            meshx_provision_failed(prov_dev, MESHX_PROVISION_FAILED_UNEXPECTED_PDU);
            ret = -MESHX_ERR_STATE;
        }
        else
        {
            prov_dev->state = MESHX_PROVISION_STATE_COMPLETE;
            /* notify app complete value */
            meshx_notify_prov_t notify_prov;
            notify_prov.metadata.prov_dev = prov_dev;
            notify_prov.metadata.notify_type = MESHX_PROV_NOTIFY_COMPLETE;
            notify_prov.pdata = NULL;
            meshx_notify(prov_dev->bearer, MESHX_NOTIFY_TYPE_PROV, &notify_prov,
                         sizeof(meshx_notify_prov_metadata_t));
        }
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
