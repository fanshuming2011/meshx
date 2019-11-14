/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "MESHX_BEARER_GATT"
#include "meshx_trace.h"
#include "meshx_bearer_internal.h"
#include "meshx_errno.h"
#include "meshx_assert.h"
#include "meshx_net.h"


typedef struct
{
    struct _meshx_bearer bearer;
    uint8_t conn_id;
} meshx_bearer_gatt_t;

int32_t meshx_bearer_gatt_init(void)
{
    return MESHX_SUCCESS;
}

int32_t meshx_bearer_gatt_send(meshx_bearer_t bearer, const uint8_t *pdata,
                               uint8_t len)
{
    MESHX_ASSERT(NULL != bearer);
    MESHX_ASSERT(NULL != pdata);
    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_gatt_create(uint8_t conn_id)
{
    return NULL;
}

void meshx_bearer_gatt_delete(meshx_bearer_t bearer)
{
    MESHX_ASSERT(NULL != bearer);
}

int32_t meshx_bearer_gatt_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len)
{
    MESHX_ASSERT(NULL != bearer);
    MESHX_ASSERT(NULL != pdata);
    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_gatt_get(uint8_t conn_id)
{
    return NULL;
}

uint16_t meshx_bearer_gatt_mtu_get(meshx_bearer_t bearer)
{
    return 1;
}