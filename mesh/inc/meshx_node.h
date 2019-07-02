/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NODE_H_
#define _MESHX_NODE_H_

#include "meshx_common.h"

MESHX_BEGIN_DECLS

typedef enum
{
    MESHX_NODE_UNPROV,
    MESHX_NODE_PROVING,
    MESHX_NODE_PROVED
} meshx_node_provision_state_t;

typedef enum
{
    MESHX_NODE_PARAM_TYPE_DEV_UUID,
    MESHX_NODE_PARAM_TYPE_UDB_INTERVAL,
    MESHX_NODE_PARAM_TYPE_SNB_INTERVAL,
} meshx_node_param_type_t;

typedef struct
{
    meshx_node_param_type_t type;
    union
    {
        meshx_dev_uuid_t dev_uuid;
        uint32_t udb_interval;
        uint32_t snb_interval;
    };
} meshx_node_param_t;


MESHX_EXTERN uint16_t meshx_get_node_address(void);
MESHX_EXTERN void meshx_set_node_prov_state(meshx_node_provision_state_t prov_state);
MESHX_EXTERN meshx_node_provision_state_t meshx_get_node_prov_state(void);

MESHX_EXTERN int32_t meshx_node_param_set(const meshx_node_param_t *pparam);
MESHX_EXTERN int32_t meshx_node_param_get(meshx_node_param_type_t type, void *pdata);

MESHX_END_DECLS

#endif

