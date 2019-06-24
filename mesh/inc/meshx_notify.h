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


#define MESHX_PROV_NOTIFY_LINK_OPEN            0x00 /* @ref meshx_provision_link_open_t */
#define MESHX_PROV_NOTIFY_LINK_CLOSE           0x01 /* @ref meshx_provision_link_close_t */
#define MESHX_PROV_NOTIFY_INVITE               0x02 /* @ref meshx_provision_invite_t */
#define MESHX_PROV_NOTIFY_CAPABILITES          0x03 /* @ref meshx_provision_capabilites_t */
#define MESHX_PROV_NOTIFY_START                0x04 /* @ref meshx_provision_start_t */
#define MESHX_PROV_NOTIFY_FAILED               0xfe /* @ref meshx provisison failed error code macros */
#define MESHX_PROV_NOTIFY_COMPLETE             0xff /* @ref NULL */

typedef struct
{
    meshx_provision_dev_t prov_dev;
    uint8_t prov_type;
} __PACKED meshx_notify_prov_metadata_t;

typedef struct
{
    meshx_notify_prov_metadata_t metadata;
    union
    {
        meshx_provision_link_close_reason_t link_close_reason;
        meshx_provision_link_open_result_t link_open_result;
        meshx_provision_invite_t invite;
        meshx_provision_capabilites_t capabilites;
        meshx_provision_start_t start;
        uint8_t prov_failed_reason;
    };
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


typedef int32_t (*meshx_notify_t)(uint8_t notify_type, const void *pdata, uint8_t len);
MESHX_EXTERN int32_t meshx_notify_init(meshx_notify_t notify_func);

MESHX_END_DECLS


#endif /* _MESHX_NOTIFY_H_ */
