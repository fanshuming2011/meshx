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
#include "meshx_list.h"
#include "meshx_proxy.h"
#include "meshx_gap_wrapper.h"


typedef struct
{
    struct _meshx_bearer bearer;
    uint16_t conn_handle;
    meshx_list_t node;
} meshx_bearer_gatt_t;

static meshx_list_t meshx_bearer_gatt_list;

meshx_bearer_t meshx_bearer_gatt_get(uint16_t conn_handle)
{
    meshx_list_t *pnode;
    meshx_bearer_gatt_t *pbearer;
    meshx_list_foreach(pnode, &meshx_bearer_gatt_list)
    {
        pbearer = MESHX_CONTAINER_OF(pnode, meshx_bearer_gatt_t, node);
        if (pbearer->conn_handle == conn_handle)
        {
            return &pbearer->bearer;
        }
    }

    return NULL;
}

static bool meshx_bearer_gatt_exists(meshx_bearer_t bearer)
{
    meshx_list_t *pnode;
    meshx_bearer_gatt_t *pbearer;
    meshx_list_foreach(pnode, &meshx_bearer_gatt_list)
    {
        pbearer = MESHX_CONTAINER_OF(pnode, meshx_bearer_gatt_t, node);
        if (&pbearer->bearer == bearer)
        {
            return TRUE;
        }
    }

    return FALSE;
}

int32_t meshx_bearer_gatt_init(void)
{
    meshx_list_init_head(&meshx_bearer_gatt_list);
    return MESHX_SUCCESS;
}

int32_t meshx_bearer_gatt_send(meshx_bearer_t bearer, const uint8_t *pdata,
                               uint16_t len)
{
#if MESHX_REDUNDANCY_CHECK
    if (!meshx_bearer_gatt_exists(bearer))
    {
        MESHX_ERROR("invalid bearer: 0x%08x", bearer);
        return -MESHX_ERR_INVAL;
    }
#endif

    //meshx_gap_gatts_send(conn_handle, pdata, len);
    return MESHX_SUCCESS;
}

meshx_bearer_t meshx_bearer_gatt_create(uint16_t conn_handle)
{
    return NULL;
}

void meshx_bearer_gatt_delete(meshx_bearer_t bearer)
{
    MESHX_ASSERT(NULL != bearer);
}

int32_t meshx_bearer_gatt_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint16_t len)
{
    if (NULL == bearer)
    {
        MESHX_ERROR("invalid bearer!");
        return -MESHX_ERR_INVAL;
    }
    MESHX_ASSERT(NULL != pdata);
    return MESHX_SUCCESS;
}

uint16_t meshx_bearer_gatt_mtu_get(meshx_bearer_t bearer)
{
    return 1;
}