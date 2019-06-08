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
#include "meshx_provision_callback.h"

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
#define MESHX_PROVISION_FAILED_UNEXPPECTED_PDU             0x03
#define MESHX_PROVISION_FAILED_CONFIMATION_FAILED          0x04
#define MESHX_PROVISION_FAILED_OUT_OF_RESOURCE             0x05
#define MESHX_PROVISION_FAILED_DECRYPTION_FAILED           0x06
#define MESHX_PROVISION_FAILED_UNEXPECTED_ERROR            0x07
#define MESHX_PROVISION_FAILED_CANNOT_ASSIGN_ADDRESS       0x08


typedef struct
{
    uint8_t type : 6;
    uint8_t padding : 2;
} __PACKED meshx_provision_pdu_metadata_t;

typedef struct
{
    uint8_t attention_duration;
} __PACKED meshx_provision_invite_t;

typedef struct
{
    uint8_t element_nums;
    uint16_t algorithms;
    uint8_t public_key_type;
    uint8_t static_oob_type;
    uint8_t output_oob_type;
    uint16_t output_oob_action;
    uint8_t input_oob_size;
    uint8_t input_oob_action;
} __PACKED meshx_provision_capabilites_t;

typedef struct
{
    meshx_provision_pdu_metadata_t metadata;
    union
    {
        meshx_provision_invite_t invite;
        meshx_provision_capabilites_t capabilites;
    };
} __PACKED meshx_provision_pdu_t;

typedef struct _meshx_provision_dev *meshx_provision_dev_t;

typedef int32_t (*meshx_provision_callback_t)(const meshx_provision_dev_t prov_dev, uint8_t type,
                                              void *pargs);

MESHX_EXTERN int32_t meshx_provision_init(meshx_provision_callback_t pcb);

MESHX_EXTERN meshx_provision_dev_t meshx_provision_create_device(meshx_bearer_t bearer,
                                                                 meshx_dev_uuid_t dev_uuid);

MESHX_EXTERN int32_t meshx_provision_link_open(meshx_provision_dev_t prov_dev);
MESHX_EXTERN int32_t meshx_provision_link_close(meshx_provision_dev_t prov_dev, uint8_t reason);

MESHX_EXTERN int32_t meshx_provision_invite(meshx_provision_dev_t prov_dev,
                                            meshx_provision_invite_t invite);
MESHX_EXTERN int32_t meshx_provision_capabilites(meshx_provision_dev_t prov_dev,
                                                 const meshx_provision_capabilites_t *pcap);

MESHX_END_DECLS


#endif /* _MESHX_PROVISION_H_ */
