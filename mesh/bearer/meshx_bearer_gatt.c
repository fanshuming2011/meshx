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
#include "meshx_assert.h"
#include "meshx_network.h"


typedef struct
{
    struct _meshx_bearer bearer;
    meshx_bearer_param_gatt_t param;
} meshx_bearer_gatt_t;

int32_t meshx_bearer_gatt_init(void)
{
    return MESHX_SUCCESS;
}

int32_t meshx_bearer_gatt_send(meshx_bearer_t bearer, uint8_t pkt_type, const uint8_t *pdata,
                               uint8_t len)
{
    MESHX_ASSERT(NULL != bearer);
    MESHX_ASSERT(NULL != pdata);
    MESHX_ASSERT(MESHX_IS_BEARER_GATT_ADV_PKT_TYPE_VALID(pkt_type));
    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_gatt_create(meshx_bearer_param_gatt_t gatt_param)
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

meshx_bearer_t meshx_bearer_gatt_get(const meshx_bearer_rx_metadata_gatt_t *prx_metadata)
{
    return NULL;
}
