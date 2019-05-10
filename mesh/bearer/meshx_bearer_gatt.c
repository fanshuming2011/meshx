/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_BEARER_ADV"
#include "meshx_trace.h"
#include "meshx_bearer_internal.h"
#include "meshx_errno.h"


int32_t meshx_bearer_gatt_init(void)
{
    return MESHX_SUCCESS;
}

int32_t meshx_bearer_gatt_send(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_gatt_create(meshx_bearer_param_gatt_t gatt_param)
{
    meshx_bearer_t bearer = {.bearer = 0};
    return bearer;
}

void meshx_bearer_gatt_delete(meshx_bearer_t bearer)
{

}

bool meshx_bearer_gatt_is_valid(meshx_bearer_t bearer)
{
    return FALSE;
}

int32_t meshx_bearer_gatt_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_gatt_get(const meshx_bearer_rx_metadata_gatt_t *prx_metadata)
{
    meshx_bearer_t bearer = {0};
    return bearer;
}
