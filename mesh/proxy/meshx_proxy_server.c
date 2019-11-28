/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "MESHX_PROXY_SERVER"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_gatt.h"
#include "meshx_proxy.h"
#include "meshx_bearer.h"

#define MESHX_PROXY_CHAR_DATA_IN_INDEX                  0
#define MESHX_PROXY_CHAR_DATA_OUT_INDEX                 1

static meshx_gatts_char_db_t proxy_char[2] =
{
    {
        .char_uuid = (meshx_gatts_uuid_t){.is_uuid16 = TRUE, .uuid16 = MESHX_GATT_UUID_CHAR_PROXY_DATA_IN},
        .char_property = MESHX_CHAR_PROPERTY_WRITE_WITHOUT_RESP,
        .pvalue = NULL,
        .char_value_len = 0,
        .is_variable_len = TRUE,
        .char_value_handle = MESHX_GATT_HANDLE_INVALID,
    },
    {
        .char_uuid = (meshx_gatts_uuid_t){.is_uuid16 = TRUE, .uuid16 = MESHX_GATT_UUID_CHAR_PROXY_DATA_OUT},
        .char_property = MESHX_CHAR_PROPERTY_NOTIFY,
        .pvalue = NULL,
        .char_value_len = 0,
        .is_variable_len = TRUE,
        .char_value_handle = MESHX_GATT_HANDLE_INVALID,
    }
};

static meshx_gatts_srv_db_t proxy_service =
{
    .srv_type = MESHX_PRIMARY_SERVICE,
    .srv_uuid = (meshx_gatts_uuid_t){.is_uuid16 = TRUE, .uuid16 = MESHX_GATT_UUID_SERVICE_PROXY},
    .char_num = sizeof(proxy_char) / sizeof(meshx_gatts_char_db_t),
    .pchar_db = proxy_char,
    .srv_handle = MESHX_GATT_HANDLE_INVALID,
};

static meshx_gatts_db_t proxy_service_db =
{
    .srv_num = 1,
    .psrv_db = &proxy_service,
};

int32_t meshx_proxy_server_init(void)
{
    return meshx_gatts_service_init(&proxy_service_db);
}

void meshx_proxy_server_deinit(void)
{

}

uint16_t meshx_proxy_char_data_out_handle(void)
{
    return proxy_service.pchar_db[MESHX_PROXY_CHAR_DATA_OUT_INDEX].char_value_handle;
}

uint16_t meshx_proxy_char_data_in_handle(void)
{
    return proxy_service.pchar_db[MESHX_PROXY_CHAR_DATA_IN_INDEX].char_value_handle;
}

int32_t meshx_proxy_server_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint16_t len)
{
    return meshx_proxy_receive(bearer, pdata, len);
}

int32_t meshx_proxy_server_send(meshx_bearer_t bearer, const uint8_t *pdata, uint16_t len)
{
    return MESHX_SUCCESS;
}

