/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_PROVISION_H_
#define _MESHX_PROVISION_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS

#define MESHX_PROVISION_TYPE_INVITE                  0x00
#define MESHX_PROVISION_TYPE_CAPABILITES             0x01
#define MESHX_PROVISION_TYPE_START                   0x02
#define MESHX_PROVISION_TYPE_PUBLIC_KEY              0x03
#define MESHX_PROVISION_TYPE_INPUT_COMPLETE          0x04
#define MESHX_PROVISION_TYPE_CONFIRMATION            0x05
#define MESHX_PROVISION_TYPE_RANDOM                  0x06
#define MESHX_PROVISION_TYPE_DATA                    0x07
#define MESHX_PROVISION_TYPE_COMPLETE                0x08
#define MESHX_PROVISION_TYPE_FAILED                  0x09

#define MESHX_PROVISION_FAILED_PROHIBITED                  0x00
#define MESHX_PROVISION_FAILED_INVALID_PDU                 0x01
#define MESHX_PROVISION_FAILED_INVALID_FORMAT              0x02
#define MESHX_PROVISION_FAILED_UNEXPECTED_PDU              0x03
#define MESHX_PROVISION_FAILED_CONFIMATION_FAILED          0x04
#define MESHX_PROVISION_FAILED_OUT_OF_RESOURCE             0x05
#define MESHX_PROVISION_FAILED_DECRYPTION_FAILED           0x06
#define MESHX_PROVISION_FAILED_UNEXPECTED_ERROR            0x07
#define MESHX_PROVISION_FAILED_CANNOT_ASSIGN_ADDRESS       0x08

typedef enum
{
    MESHX_PROVISION_STATE_IDLE,
    MESHX_PROVISION_STATE_LINK_OPENING,
    MESHX_PROVISION_STATE_LINK_OPENED,
    MESHX_PROVISION_STATE_INVITE,
    MESHX_PROVISION_STATE_CAPABILITES,
    MESHX_PROVISION_STATE_START,
    MESHX_PROVISION_STATE_PUBLIC_KEY,
    MESHX_PROVISION_STATE_CONFIRMATION,
    MESHX_PROVISION_STATE_RANDOM,
    MESHX_PROVISION_STATE_DATA,
    MESHX_PROVISION_STATE_COMPLETE,
    MESHX_PROVISION_STATE_FAILED,
    MESHX_PROVISION_STATE_LINK_CLOSING,
} meshx_provision_state_t;


#define MESHX_PROVISION_LINK_CLOSE_SUCCESS         0
#define MESHX_PROVISION_LINK_CLOSE_TIMEOUT         1
#define MESHX_PROVISION_LINK_CLOSE_FAIL            2
#define MESHX_PROVISION_LINK_CLOSE_LINK_LOSS       3

typedef enum
{
    MESHX_PROVISION_LINK_OPEN_SUCCESS,
    MESHX_PROVISION_LINK_OPEN_TIMEOUT,
} meshx_provision_link_open_result_t;

typedef struct
{
    uint8_t type : 6;
    uint8_t padding : 2;
} __PACKED meshx_provision_pdu_metadata_t;

typedef struct
{
    uint8_t attention_duration;
} __PACKED meshx_provision_invite_t;

#define MESHX_PROVISION_CAP_ALGORITHM_P256_CURVE                    0x01
#define MESHX_PROVISION_CAP_ALGORITHM_RFU                           0x02

#define MESHX_PROVISION_CAP_PUBLIC_KEY_NO_OOB                       0x00
#define MESHX_PROVISION_CAP_PUBLIC_KEY_OOB                          0x01
#define MESHX_PROVISION_CAP_PUBLIC_KEY_RFU                          0x02

#define MESHX_PROVISION_CAP_STATIC_OOB_NOT_AVAIABLE                 0x00
#define MESHX_PROVISION_CAP_STATIC_OOB_AVAIABLE                     0x01
#define MESHX_PROVISION_CAP_STATIC_OOB_RFU                          0x02

#define MESHX_PROVISION_CAP_NOT_SUPPORT_OUTPUT_OOB                  0x00
#define MESHX_PROVISION_CAP_OUTPUT_OOB_SIZE_MIN                     0x01
#define MESHX_PROVISION_CAP_OUTPUT_OOB_SIZE_MAX                     0x08
#define MESHX_PROVISION_CAP_OUTPUT_OOB_SIZE_RFU                     0x09

#define MESHX_PROVISION_CAP_OUTPUT_OOB_ACTION_BLINK                 0x00
#define MESHX_PROVISION_CAP_OUTPUT_OOB_ACTION_BEEP                  0x01
#define MESHX_PROVISION_CAP_OUTPUT_OOB_ACTION_VIBRATE               0x02
#define MESHX_PROVISION_CAP_OUTPUT_OOB_ACTION_OUTPUT_NUMERIC        0x04
#define MESHX_PROVISION_CAP_OUTPUT_OOB_ACTION_OUTPUT_ALPHA          0x08
#define MESHX_PROVISION_CAP_OUTPUT_OOB_ACTION_RFU                   0x10

#define MESHX_PROVISION_CAP_NOT_SUPPORT_INPUT_OOB                   0x00
#define MESHX_PROVISION_CAP_INPUT_OOB_SIZE_MIN                      0x01
#define MESHX_PROVISION_CAP_INPUT_OOB_SIZE_MAX                      0x08
#define MESHX_PROVISION_CAP_INPUT_OOB_SIZE_RFU                      0x09

#define MESHX_PROVISION_CAP_INPUT_OOB_ACTION_PUSH                   0x00
#define MESHX_PROVISION_CAP_INPUT_OOB_ACTION_TWIST                  0x01
#define MESHX_PROVISION_CAP_INPUT_OOB_ACTION_INPUT_NUMERIC          0x02
#define MESHX_PROVISION_CAP_INPUT_OOB_ACTION_INPUT_ALPHA            0x04
#define MESHX_PROVISION_CAP_INPUT_OOB_ACTION_RFU                    0x08

typedef struct
{
    uint8_t element_nums;
    uint16_t algorithms;
    uint8_t public_key_type;
    uint8_t static_oob_type;
    uint8_t output_oob_size;
    uint16_t output_oob_action;
    uint8_t input_oob_size;
    uint16_t input_oob_action;
} __PACKED meshx_provision_capabilites_t;

#define MESHX_PROVISION_AUTH_METHOD_NO_OOB                      0x00
#define MESHX_PROVISION_AUTH_METHOD_STATIC_OOB                  0x01
#define MESHX_PROVISION_AUTH_METHOD_OUTPUT_OOB                  0x02
#define MESHX_PROVISION_AUTH_METHOD_INPUT_OOB                   0x03
#define MESHX_PROVISION_AUTH_METHOD_PROHIBITED                  0x04

#define MESHX_PROVISION_AUTH_ACTION_BLINK                       0x00
#define MESHX_PROVISION_AUTH_ACTION_BEEP                        0x01
#define MESHX_PROVISION_AUTH_ACTION_VIBRATE                     0x02
#define MESHX_PROVISION_AUTH_ACTION_OUT_NUMERIC                 0x03
#define MESHX_PROVISION_AUTH_ACTION_OUT_ALPHA                   0x04
#define MESHX_PROVISION_AUTH_ACTION_RFU                         0x05

#define MESHX_PROVISION_AUTH_SIZE_MIN                           0x01
#define MESHX_PROVISION_AUTH_SIZE_MAX                           0x08

typedef struct
{
    uint8_t algorithm;
    uint8_t public_key;
    uint8_t auth_method;
    uint8_t auth_action;
    uint8_t auth_size;
} __PACKED meshx_provision_start_t;

typedef struct
{
    uint8_t pub_key_x[32];
    uint8_t pub_key_y[32];
} __PACKED meshx_provision_public_key_t;

typedef struct
{
    uint8_t confirmation[16];
} __PACKED meshx_provision_confirmation_t;

typedef struct
{
    uint8_t random[16];
} __PACKED meshx_provision_random_t;

typedef struct
{
    uint8_t auth_value[16];
} __PACKED meshx_provision_auth_value_t;

typedef struct
{
    uint8_t network_key[16];
    uint16_t key_index;
    uint8_t key_refresh_flag : 1;
    uint8_t iv_update_flag : 1;
    uint8_t rfu : 6;
    uint32_t iv_index;
    uint16_t unicast_address;
    uint8_t mic[8];
} __PACKED meshx_provision_data_t;

typedef struct
{
    meshx_provision_pdu_metadata_t metadata;
    union
    {
        meshx_provision_invite_t invite;
        meshx_provision_capabilites_t capabilites;
        meshx_provision_start_t start;
        meshx_provision_public_key_t public_key;
        meshx_provision_confirmation_t confirmation;
        meshx_provision_random_t random;
        meshx_provision_data_t data;
        uint8_t err_code;
    };
} __PACKED meshx_provision_pdu_t;

typedef struct _meshx_provision_dev *meshx_provision_dev_t;

MESHX_EXTERN int32_t meshx_provision_init(void);

MESHX_EXTERN meshx_provision_dev_t meshx_provision_create_device(meshx_bearer_t bearer,
                                                                 meshx_dev_uuid_t dev_uuid, meshx_role_t role);

MESHX_EXTERN int32_t meshx_provision_make_key(meshx_provision_dev_t prov_dev);
MESHX_EXTERN bool meshx_provision_validate_public_key(const meshx_provision_public_key_t *pkey);
MESHX_EXTERN int32_t meshx_provision_get_local_public_key(meshx_provision_dev_t prov_dev,
                                                          meshx_provision_public_key_t *pkey);
MESHX_EXTERN int32_t meshx_provision_set_remote_public_key(meshx_provision_dev_t prov_dev,
                                                           const meshx_provision_public_key_t *pkey);
MESHX_EXTERN int32_t meshx_provision_generate_auth_value(meshx_provision_dev_t prov_dev,
                                                         const uint8_t *pauth_value, uint8_t len);
MESHX_EXTERN int32_t meshx_provision_generate_random(meshx_provision_dev_t prov_dev);
MESHX_EXTERN int32_t meshx_provision_get_random(meshx_provision_dev_t prov_dev,
                                                meshx_provision_random_t *prandom);
MESHX_EXTERN int32_t meshx_provision_generate_confirmation(meshx_provision_dev_t prov_dev,
                                                           const meshx_provision_random_t *prandom);
MESHX_EXTERN int32_t meshx_provision_get_confirmation(meshx_provision_dev_t prov_dev,
                                                      meshx_provision_confirmation_t *pcfm);


MESHX_EXTERN int32_t meshx_provision_link_open(meshx_provision_dev_t prov_dev);
MESHX_EXTERN int32_t meshx_provision_link_close(meshx_provision_dev_t prov_dev, uint8_t reason);

MESHX_EXTERN int32_t meshx_provision_invite(meshx_provision_dev_t prov_dev,
                                            meshx_provision_invite_t invite);
MESHX_EXTERN int32_t meshx_provision_capabilites(meshx_provision_dev_t prov_dev,
                                                 const meshx_provision_capabilites_t *pcap);
MESHX_EXTERN int32_t meshx_provision_start(meshx_provision_dev_t prov_dev,
                                           const meshx_provision_start_t *pstart);
MESHX_EXTERN int32_t meshx_provision_public_key(meshx_provision_dev_t prov_dev,
                                                const meshx_provision_public_key_t *ppub_key);
MESHX_EXTERN int32_t meshx_provision_confirmation(meshx_provision_dev_t prov_dev,
                                                  const meshx_provision_confirmation_t *pcfm);
MESHX_EXTERN int32_t meshx_provision_random(meshx_provision_dev_t prov_dev,
                                            const meshx_provision_random_t *prandom);
MESHX_EXTERN int32_t meshx_provision_data(meshx_provision_dev_t prov_dev,
                                          const meshx_provision_data_t *pdata);
MESHX_EXTERN int32_t meshx_provision_complete(meshx_provision_dev_t prov_dev);
MESHX_EXTERN int32_t meshx_provision_failed(meshx_provision_dev_t prov_dev, uint8_t err_code);


MESHX_END_DECLS


#endif /* _MESHX_PROVISION_H_ */
