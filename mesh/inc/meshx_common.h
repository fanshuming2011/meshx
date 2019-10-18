/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_COMMON_H_
#define _MESHX_COMMON_H_

#include "meshx_types.h"
#include "meshx_gap_wrapper.h"

MESHX_BEGIN_DECLS

/* mesh role */
typedef enum
{
    MESHX_ROLE_DEVICE = 0x01,
    MESHX_ROLE_PROVISIONER = 0x02,
    MESHX_ROLE_ALL = (MESHX_ROLE_DEVICE | MESHX_ROLE_PROVISIONER),
} meshx_role_t;

typedef enum
{
    MESHX_NODE_UNPROVED,
    MESHX_NODE_PROVED
} meshx_node_prov_state_t;


#define MESHX_ADDRESS_UNASSIGNED                     0x0000
#define MESHX_ADDRESS_ALL_PRXIES                     0xFFFC
#define MESHX_ADDRESS_ALL_FRIENDS                    0xFFFD
#define MESHX_ADDRESS_ALL_RELAYS                     0xFFFE
#define MESHX_ADDRESS_ALL_NODES                      0xFFFF　
#define MESHX_ADDRESS_IS_UNASSIGNED(address)         ((address) == MESHX_ADDRESS_UNASSIGNED)
#define MESHX_ADDRESS_IS_UNICAST(address)            ((((address) & 0x8000) == 0) && (((address) != MESHX_ADDRESS_UNASSIGNED)))
#define MESHX_ADDRESS_IS_VIRTUAL(address)            (((address) & 0xC000) == 0x8000))
#define MESHX_ADDRESS_IS_GROUP(address)              (((address) & 0xC000) == 0xC000))
#define MESHX_ADDRESS_IS_RFU(address)                (((address) >= 0xFF00) && ((address) <= 0xFFFB))
#define MESHX_ADDRESS_IS_VALID(address)              ((0 != (address)) && (!MESHX_ADDRESS_IS_RFU(address)))

typedef uint8_t meshx_dev_uuid_t[16];

/* keys */
#define MESHX_NET_KEY_INDEX_MAX                       0xFFF
#define MESHX_APP_KEY_INDEX_MAX                       0xFFF
typedef uint8_t meshx_key_t[16];
typedef uint8_t meshx_network_id_t[8];

typedef struct
{
    uint16_t net_key_index;
    meshx_key_t net_key;
    meshx_key_t identity_key;
    meshx_key_t beacon_key;
    uint8_t nid; /* least significant 6 bits */
    meshx_key_t encryption_key;
    meshx_key_t privacy_key;
    meshx_network_id_t network_id;
} meshx_network_key_t;

typedef struct
{
    uint16_t app_key_index;
    meshx_network_key_t *pnet_key_bind;
    meshx_key_t app_key;
    uint8_t aid; /* least significant 6 bits */
} meshx_application_key_t;

typedef struct
{
    uint16_t primary_addr;
    uint8_t element_num;
    meshx_key_t dev_key;
} meshx_device_key_t;

typedef struct
{
    /* common parameters */
    uint16_t src;
    uint16_t dst;
    uint32_t ttl : 7;
    uint32_t ctl : 1;
    uint32_t seq : 24;
    uint32_t iv_index;
    uint32_t seq_origin : 24;
    uint16_t force_seg : 1;
    uint16_t rsvd : 7;
    const meshx_network_key_t *pnet_key;

    union
    {
        /* access message parameters */
        struct
        {
            uint8_t akf : 1;
            uint8_t szmic : 1;
            uint8_t aid : 6;
            union
            {
                const meshx_key_t *papp_key;
                const meshx_key_t *pdev_key;
            };
        };

        /* control message parameters */
        struct
        {
            uint8_t opcode;
        };
    };
} meshx_msg_ctx_t;



#define MESHX_BEARER_TYPE_INVALID           0
#define MESHX_BEARER_TYPE_ADV               1
#define MESHX_BEARER_TYPE_GATT              2

typedef struct
{
    meshx_gap_adv_type_t adv_type;
    meshx_mac_addr_t peer_addr;
    meshx_mac_addr_type_t addr_type;
    uint8_t channel;
    int8_t rssi;
} meshx_bearer_rx_metadata_adv_t;

typedef struct
{
    uint16_t conn_id;
} meshx_bearer_rx_metadata_gatt_t;

typedef struct
{
    uint16_t bearer_type;
    union
    {
        meshx_bearer_rx_metadata_adv_t adv_metadata;
        meshx_bearer_rx_metadata_gatt_t gatt_metadata;
    };
} meshx_bearer_rx_metadata_t;

MESHX_END_DECLS

#endif
