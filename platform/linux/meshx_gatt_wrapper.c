/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include "meshx_gatt_wrapper.h"
#define MESHX_TRACE_MODULE "MESHX_GATT_WRAPPER"
#include "meshx_trace.h"
#include "meshx_errno.h"


int32_t meshx_gatts_service_init(meshx_gatts_db_t *pservice_db)
{
    return MESHX_SUCCESS;
}

int32_t meshx_gatts_notify(uint16_t conn_handle, uint16_t char_value_handle,
                           const uint8_t *pdata, uint16_t len)
{
    return MESHX_SUCCESS;
}

