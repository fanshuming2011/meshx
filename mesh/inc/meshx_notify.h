/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NOTIFY_H_
#define _MESHX_NOTIFY_H_

#include "meshx_provision.h"
#include "meshx_beacon.h"

MESHX_BEGIN_DECLS


typedef enum
{
    MESHX_PROV_NOTIFY_LINK_OPEN, /* @ref meshx_provision_link_open_t */
    MESHX_PROV_NOTIFY_LINK_CLOSE, /* @ref meshx_provision_link_close_t */
    MESHX_PROV_NOTIFY_TRANS_ACK, /* @ref meshx_provision_state_t */
    MESHX_PROV_NOTIFY_INVITE, /* @ref meshx_provision_invite_t */
    MESHX_PROV_NOTIFY_CAPABILITES, /* @ref meshx_provision_capabilites_t */
    MESHX_PROV_NOTIFY_START, /* @ref meshx_provision_start_t */
    MESHX_PROV_NOTIFY_PUBLIC_KEY, /* @ref meshx_provision_public_key_t */
    MESHX_PROV_NOTIFY_INPUT_COMPLETE, /* @ref NULL */
    MESHX_PROV_NOTIFY_CONFIRMATION, /* @ref meshx_provision_confirmation_t */
    MESHX_PROV_NOTIFY_RANDOM, /* @ref meshx_provision_random_t */
    MESHX_PROV_NOTIFY_DATA, /* @ref meshx_provision_data_t */
    MESHX_PROV_NOTIFY_COMPLETE, /* @ref NULL */
    MESHX_PROV_NOTIFY_FAILED, /* @ref meshx provisison failed error code macros */
} meshx_prov_notify_type_t;

typedef struct
{
    meshx_provision_dev_t prov_dev;
    /* TODO: why can not enum? */
    //meshx_prov_notify_type_t notify_type;
    uint8_t notify_type;
} __PACKED meshx_notify_prov_metadata_t;

typedef struct
{
    meshx_notify_prov_metadata_t metadata;
    const void *pdata;
} __PACKED meshx_notify_prov_t;

typedef enum
{
    MESHX_NOTIFY_BEACON_TYPE_UDB,
    MESHX_NOTIFY_BEACON_TYPE_PB_GATT,
    MESHX_NOTIFY_BEACON_TYPE_PROXY,
} meshx_notify_beacon_type_t;

typedef struct
{
    meshx_notify_beacon_type_t type;
    union
    {
        meshx_udb_t udb;
    };
    const meshx_bearer_rx_metadata_adv_t *padv_metadata;
} meshx_notify_beacon_t;


#define MESHX_NOTIFY_TYPE_PROV                0 /* @ref meshx_notify_prov_t */
#define MESHX_NOTIFY_TYPE_BEACON              1 /* @ref meshx_notify_beacon_t */


typedef int32_t (*meshx_notify_t)(meshx_bearer_t bearer, uint8_t notify_type, const void *pdata,
                                  uint8_t len);
MESHX_EXTERN int32_t meshx_notify_init(meshx_notify_t notify_func);

MESHX_END_DECLS


#endif /* _MESHX_NOTIFY_H_ */
