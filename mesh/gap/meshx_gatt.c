/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "MESHX_GATT"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_gatt.h"
#include "meshx_proxy_internal.h"
#include "meshx_bearer.h"

void meshx_gatts_event_process(meshx_gatts_evt_t evt, meshx_gatts_evt_param_t *pparam)
{
    switch (evt)
    {
    case MESHX_GATTS_EVT_WRITE:
        pparam->result = MESHX_GATTS_RESULT_SUCCESS;
        meshx_bearer_gatt_receive(meshx_bearer_gatt_get(pparam->conn_handle),
                                  pparam->write.char_value_handle, pparam->write.pdata, pparam->write.len);
        break;
    case MESHX_GATTS_EVT_READ:
        break;
    case MESHX_GATTS_EVT_WRITE_PERMIT_REQ:
        break;
    case MESHX_GATTS_EVT_READ_PERMIT_REQ:
        break;
    case MESHX_GATTS_EVT_IND_CFM:
        break;
    default:
        break;
    }
}