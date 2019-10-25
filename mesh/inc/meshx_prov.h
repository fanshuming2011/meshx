/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_PROV_H_
#define _MESHX_PROV_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS

#define MESHX_PROV_TYPE_INVITE                  0x00
#define MESHX_PROV_TYPE_CAPABILITES             0x01
#define MESHX_PROV_TYPE_START                   0x02
#define MESHX_PROV_TYPE_PUBLIC_KEY              0x03
#define MESHX_PROV_TYPE_INPUT_COMPLETE          0x04
#define MESHX_PROV_TYPE_CONFIRMATION            0x05
#define MESHX_PROV_TYPE_RANDOM                  0x06
#define MESHX_PROV_TYPE_DATA                    0x07
#define MESHX_PROV_TYPE_COMPLETE                0x08
#define MESHX_PROV_TYPE_FAILED                  0x09

#define MESHX_PROV_FAILED_PROHIBITED                  0x00
#define MESHX_PROV_FAILED_INVALID_PDU                 0x01
#define MESHX_PROV_FAILED_INVALID_FORMAT              0x02
#define MESHX_PROV_FAILED_UNEXPECTED_PDU              0x03
#define MESHX_PROV_FAILED_CONFIMATION_FAILED          0x04
#define MESHX_PROV_FAILED_OUT_OF_RESOURCE             0x05
#define MESHX_PROV_FAILED_DECRYPTION_FAILED           0x06
#define MESHX_PROV_FAILED_UNEXPECTED_ERROR            0x07
#define MESHX_PROV_FAILED_CANNOT_ASSIGN_ADDRESS       0x08

typedef enum
{
    MESHX_PROV_STATE_IDLE,
    MESHX_PROV_STATE_LINK_OPENING,
    MESHX_PROV_STATE_LINK_OPENED,
    MESHX_PROV_STATE_INVITE,
    MESHX_PROV_STATE_CAPABILITES,
    MESHX_PROV_STATE_START,
    MESHX_PROV_STATE_PUBLIC_KEY,
    MESHX_PROV_STATE_INPUT_COMPLETE,
    MESHX_PROV_STATE_CONFIRMATION,
    MESHX_PROV_STATE_RANDOM,
    MESHX_PROV_STATE_DATA,
    MESHX_PROV_STATE_COMPLETE,
    MESHX_PROV_STATE_FAILED,
    MESHX_PROV_STATE_LINK_CLOSING,
} meshx_prov_state_t;


#define MESHX_PROV_LINK_CLOSE_SUCCESS         0
#define MESHX_PROV_LINK_CLOSE_TIMEOUT         1
#define MESHX_PROV_LINK_CLOSE_FAIL            2
#define MESHX_PROV_LINK_CLOSE_LINK_LOSS       3

typedef enum
{
    MESHX_PROV_LINK_OPEN_SUCCESS,
    MESHX_PROV_LINK_OPEN_TIMEOUT,
} meshx_prov_link_open_result_t;

typedef struct
{
    uint8_t type : 6;
    uint8_t padding : 2;
} __PACKED meshx_prov_pdu_metadata_t;

typedef struct
{
    uint8_t attention_duration;
} __PACKED meshx_prov_invite_t;

#define MESHX_PROV_CAP_ALGORITHM_P256_CURVE                    0x01
#define MESHX_PROV_CAP_ALGORITHM_RFU                           0x02

#define MESHX_PROV_CAP_PUBLIC_KEY_NO_OOB                       0x00
#define MESHX_PROV_CAP_PUBLIC_KEY_OOB                          0x01
#define MESHX_PROV_CAP_PUBLIC_KEY_RFU                          0x02

#define MESHX_PROV_CAP_STATIC_OOB_NOT_AVAIABLE                 0x00
#define MESHX_PROV_CAP_STATIC_OOB_AVAIABLE                     0x01
#define MESHX_PROV_CAP_STATIC_OOB_RFU                          0x02

#define MESHX_PROV_CAP_NOT_SUPPORT_OUTPUT_OOB                  0x00
#define MESHX_PROV_CAP_OUTPUT_OOB_SIZE_MIN                     0x01
#define MESHX_PROV_CAP_OUTPUT_OOB_SIZE_MAX                     0x08
#define MESHX_PROV_CAP_OUTPUT_OOB_SIZE_RFU                     0x09

#define MESHX_PROV_CAP_OUTPUT_OOB_ACTION_BLINK                 0x00
#define MESHX_PROV_CAP_OUTPUT_OOB_ACTION_BEEP                  0x01
#define MESHX_PROV_CAP_OUTPUT_OOB_ACTION_VIBRATE               0x02
#define MESHX_PROV_CAP_OUTPUT_OOB_ACTION_OUTPUT_NUMERIC        0x04
#define MESHX_PROV_CAP_OUTPUT_OOB_ACTION_OUTPUT_ALPHA          0x08
#define MESHX_PROV_CAP_OUTPUT_OOB_ACTION_RFU                   0x10

#define MESHX_PROV_CAP_NOT_SUPPORT_INPUT_OOB                   0x00
#define MESHX_PROV_CAP_INPUT_OOB_SIZE_MIN                      0x01
#define MESHX_PROV_CAP_INPUT_OOB_SIZE_MAX                      0x08
#define MESHX_PROV_CAP_INPUT_OOB_SIZE_RFU                      0x09

#define MESHX_PROV_CAP_INPUT_OOB_ACTION_PUSH                   0x00
#define MESHX_PROV_CAP_INPUT_OOB_ACTION_TWIST                  0x01
#define MESHX_PROV_CAP_INPUT_OOB_ACTION_INPUT_NUMERIC          0x02
#define MESHX_PROV_CAP_INPUT_OOB_ACTION_INPUT_ALPHA            0x04
#define MESHX_PROV_CAP_INPUT_OOB_ACTION_RFU                    0x08

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
} __PACKED meshx_prov_capabilites_t;

#define MESHX_PROV_ALGORITHM_P256_CURVE                  0x01

#define MESHX_PROV_PUBLIC_KEY_NO_OOB                     0x00
#define MESHX_PROV_PUBLIC_KEY_OOB                        0x01

#define MESHX_PROV_AUTH_METHOD_NO_OOB                    0x00
#define MESHX_PROV_AUTH_METHOD_STATIC_OOB                0x01
#define MESHX_PROV_AUTH_METHOD_OUTPUT_OOB                0x02
#define MESHX_PROV_AUTH_METHOD_INPUT_OOB                 0x03
#define MESHX_PROV_AUTH_METHOD_PROHIBITED                0x04

/* output oob action */
#define MESHX_PROV_AUTH_ACTION_BLINK                     0x00
#define MESHX_PROV_AUTH_ACTION_BEEP                      0x01
#define MESHX_PROV_AUTH_ACTION_VIBRATE                   0x02
#define MESHX_PROV_AUTH_ACTION_OUTPUT_NUMERIC            0x03
#define MESHX_PROV_AUTH_ACTION_OUTPUT_ALPHA              0x04
#define MESHX_PROV_AUTH_ACTION_OUTPUT_RFU                0x05

/* input oob action */
#define MESHX_PROV_AUTH_ACTION_PUSH                      0x00
#define MESHX_PROV_AUTH_ACTION_TWIST                     0x01
#define MESHX_PROV_AUTH_ACTION_INPUT_NUMERIC             0x02
#define MESHX_PROV_AUTH_ACTION_INPUT_ALPHA               0x03
#define MESHX_PROV_AUTH_ACTION_INPUT_RFU                 0x04

#define MESHX_PROV_AUTH_SIZE_MIN                         0x01
#define MESHX_PROV_AUTH_SIZE_MAX                         0x08

typedef struct
{
    uint8_t algorithm;
    uint8_t public_key;
    uint8_t auth_method;
    uint8_t auth_action;
    uint8_t auth_size;
} __PACKED meshx_prov_start_t;

typedef struct
{
    uint8_t pub_key_x[32];
    uint8_t pub_key_y[32];
} __PACKED meshx_prov_public_key_t;

typedef struct
{
    uint8_t confirmation[16];
} __PACKED meshx_prov_confirmation_t;

typedef struct
{
    uint8_t random[16];
} __PACKED meshx_prov_random_t;

typedef struct
{
    uint8_t auth_value[16];
} meshx_prov_auth_value_static_oob_t;

typedef struct
{
    uint8_t auth_action;
    union
    {
        uint32_t auth_value_numeric;
        struct
        {
            uint8_t auth_value_alpha[8];
            uint8_t auth_value_alpha_len;
        };
    };
} meshx_prov_auth_value_oob_t;

typedef struct
{
    uint8_t auth_method;
    union
    {
        meshx_prov_auth_value_static_oob_t static_oob;
        meshx_prov_auth_value_oob_t oob;
    };
} meshx_prov_auth_value_t;

typedef struct
{
    meshx_key_t net_key;
    uint16_t key_index;
    uint8_t key_refresh_flag : 1;
    uint8_t iv_update_flag : 1;
    uint8_t rfu : 6;
    uint32_t iv_index;
    uint16_t unicast_address;
    uint8_t mic[8];
} __PACKED meshx_prov_data_t;

typedef struct
{
    meshx_prov_pdu_metadata_t metadata;
    union
    {
        meshx_prov_invite_t invite;
        meshx_prov_capabilites_t capabilites;
        meshx_prov_start_t start;
        meshx_prov_public_key_t public_key;
        meshx_prov_confirmation_t confirmation;
        meshx_prov_random_t random;
        meshx_prov_data_t data;
        uint8_t err_code;
    };
} __PACKED meshx_prov_pdu_t;

typedef struct _meshx_prov_dev *meshx_prov_dev_t;

MESHX_EXTERN int32_t meshx_prov_init(void);

MESHX_EXTERN meshx_prov_dev_t meshx_prov_create_device(meshx_bearer_t bearer,
                                                       meshx_dev_uuid_t dev_uuid, meshx_role_t role);
MESHX_EXTERN uint8_t meshx_prov_get_device_id(meshx_prov_dev_t prov_dev);

MESHX_EXTERN int32_t meshx_prov_make_key(meshx_prov_dev_t prov_dev);
MESHX_EXTERN bool meshx_prov_validate_public_key(const meshx_prov_public_key_t *pkey);
MESHX_EXTERN int32_t meshx_prov_get_local_public_key(meshx_prov_dev_t prov_dev,
                                                     meshx_prov_public_key_t *pkey);
MESHX_EXTERN int32_t meshx_prov_set_remote_public_key(meshx_prov_dev_t prov_dev,
                                                      const meshx_prov_public_key_t *pkey);
MESHX_EXTERN int32_t meshx_prov_set_auth_value(meshx_prov_dev_t prov_dev,
                                               const meshx_prov_auth_value_t *pauth_value);

MESHX_EXTERN int32_t meshx_prov_generate_random(meshx_prov_dev_t prov_dev);
MESHX_EXTERN int32_t meshx_prov_get_random(meshx_prov_dev_t prov_dev,
                                           meshx_prov_random_t *prandom);
MESHX_EXTERN int32_t meshx_prov_generate_confirmation(meshx_prov_dev_t prov_dev,
                                                      const meshx_prov_random_t *prandom);
MESHX_EXTERN int32_t meshx_prov_get_confirmation(meshx_prov_dev_t prov_dev,
                                                 meshx_prov_confirmation_t *pcfm);

MESHX_EXTERN uint8_t meshx_prov_get_auth_method(meshx_prov_dev_t prov_dev);
MESHX_EXTERN uint8_t meshx_prov_get_auth_action(meshx_prov_dev_t prov_dev);

MESHX_EXTERN int32_t meshx_prov_link_open(meshx_prov_dev_t prov_dev);
MESHX_EXTERN int32_t meshx_prov_link_close(meshx_prov_dev_t prov_dev, uint8_t reason);

MESHX_EXTERN int32_t meshx_prov_invite(meshx_prov_dev_t prov_dev,
                                       meshx_prov_invite_t invite);
MESHX_EXTERN int32_t meshx_prov_capabilites(meshx_prov_dev_t prov_dev,
                                            const meshx_prov_capabilites_t *pcap);
MESHX_EXTERN int32_t meshx_prov_start(meshx_prov_dev_t prov_dev,
                                      const meshx_prov_start_t *pstart);
MESHX_EXTERN int32_t meshx_prov_public_key(meshx_prov_dev_t prov_dev,
                                           const meshx_prov_public_key_t *ppub_key);
MESHX_EXTERN int32_t meshx_prov_input_complete(meshx_prov_dev_t prov_dev);
MESHX_EXTERN int32_t meshx_prov_confirmation(meshx_prov_dev_t prov_dev,
                                             const meshx_prov_confirmation_t *pcfm);
MESHX_EXTERN int32_t meshx_prov_random(meshx_prov_dev_t prov_dev,
                                       const meshx_prov_random_t *prandom);
MESHX_EXTERN int32_t meshx_prov_data(meshx_prov_dev_t prov_dev,
                                     const meshx_prov_data_t *pdata);
MESHX_EXTERN int32_t meshx_prov_complete(meshx_prov_dev_t prov_dev);
MESHX_EXTERN int32_t meshx_prov_failed(meshx_prov_dev_t prov_dev, uint8_t err_code);


MESHX_END_DECLS


#endif /* _MESHX_PROV_H_ */
