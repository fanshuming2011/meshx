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

#define MESHX_ADDRESS_UNASSIGNED                     0x0000
#define MESHX_ADDRESS_ALL_PRXIES                     0xFFFC
#define MESHX_ADDRESS_ALL_FRIENDS                    0xFFFD
#define MESHX_ADDRESS_ALL_RELAYS                     0xFFFE
#define MESHX_ADDRESS_ALL_NODES                      0xFFFF　
#define MESHX_ADDRESS_IS_UNASSIGNED(address)         ((address) == MESHX_UNASSIGNED_ADDRESS)
#define MESHX_ADDRESS_IS_UNICAST(address)            ((((address) & 0x8000) == 0) && ((address != MESHX_UNASSIGNED_ADDRESS)))
#define MESHX_ADDRESS_IS_VIRTUAL(address)            (((address) & 0xC000) == 0x8000))
#define MESHX_ADDRESS_IS_GROUP(address)              (((address) & 0xC000) == 0xC000))

typedef uint8_t meshx_dev_uuid_t[16];


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
    uint8_t conn_id;
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
