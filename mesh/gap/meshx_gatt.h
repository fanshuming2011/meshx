/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_GATT_H_
#define _MESHX_GATT_H_

#include "meshx_common.h"
#include "meshx_gatt_wrapper.h"

MESHX_BEGIN_DECLS

#define MESHX_GATT_UUID_SERVICE_PROV                           0x1827
#define MESHX_GATT_UUID_CHAR_PROV_DATA_IN                      0x2ADB
#define MESHX_GATT_UUID_CHAR_PROV_DATA_OUT                     0x2ADC

#define MESHX_GATT_UUID_SERVICE_PROXY                          0x1828
#define MESHX_GATT_UUID_CHAR_PROXY_DATA_IN                     0x2ADD
#define MESHX_GATT_UUID_CHAR_PROXY_DATA_OUT                    0x2ADE

#define MESHX_GATT_HANDLE_INVALID                              0xFFFF

typedef enum
{
    MESHX_GATTS_EVT_WRITE,
    MESHX_GATTS_EVT_READ,
    MESHX_GATTS_EVT_WRITE_PERMIT_REQ,
    MESHX_GATTS_EVT_READ_PERMIT_REQ,
    MESHX_GATTS_EVT_IND_CFM,
} meshx_gatts_evt_t;

typedef struct
{
    uint16_t char_value_handle;
    uint8_t *pdata;
    uint16_t len;
} meshx_gatts_write_t;

typedef struct
{
    uint16_t char_value_handle;
} meshx_gatts_read_t;

#define MESHX_GATTS_RESULT_SUCCESS                0
#define MESHX_GATTS_RESULT_REJECT                 1 /* no permit */

typedef struct
{
    uint16_t conn_handle;
    union
    {
        meshx_gatts_write_t write;
        meshx_gatts_read_t read;
    };
    uint8_t result;
} meshx_gatts_evt_param_t;

void meshx_gatts_event_process(meshx_gatts_evt_t evt, meshx_gatts_evt_param_t *pparam);

MESHX_END_DECLS

#endif /* _MESHX_GATT_H_ */
