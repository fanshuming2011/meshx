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
    MESHX_NODE_PARAM_TYPE_DEV_UUID,
    MESHX_NODE_PARAM_TYPE_UDB_INTERVAL,
    MESHX_NODE_PARAM_TYPE_SNB_INTERVAL,
    MESHX_NODE_PARAM_TYPE_NET_KEY_NUM,
    MESHX_NODE_PARAM_TYPE_APP_KEY_NUM,
} meshx_node_param_type_t;

typedef struct
{
    meshx_node_param_type_t type;
    union
    {
        meshx_dev_uuid_t dev_uuid;
        uint32_t udb_interval;
        uint32_t snb_interval;
        uint16_t net_key_num;
        uint16_t app_key_num;
    };
} meshx_node_param_t;



MESHX_EXTERN uint16_t meshx_node_address_get(void);
MESHX_EXTERN void meshx_node_prov_state_set(meshx_node_prov_state_t prov_state);
MESHX_EXTERN meshx_node_prov_state_t meshx_node_prov_state_get(void);

MESHX_EXTERN int32_t meshx_node_param_set(const meshx_node_param_t *pparam);
MESHX_EXTERN int32_t meshx_node_param_get(meshx_node_param_type_t type, void *pdata);

MESHX_EXTERN const meshx_application_key_t *meshx_app_key_get(uint16_t app_key_index);
MESHX_EXTERN int32_t meshx_app_key_add(uint16_t net_key_index, uint16_t app_key_index,
                                       meshx_app_key_t app_key);
MESHX_EXTERN int32_t meshx_app_key_update(uint16_t net_key_index, uint16_t app_key_index,
                                          meshx_app_key_t app_key);
MESHX_EXTERN int32_t meshx_app_key_delte(uint16_t net_key_index, uint16_t app_key_index);
MESHX_EXTERN void meshx_app_key_clear(void);

MESHX_EXTERN const meshx_network_key_t *meshx_net_key_get(uint16_t net_key_index);
MESHX_EXTERN int32_t meshx_net_key_add(uint16_t net_key_index, meshx_net_key_t net_key);
MESHX_EXTERN int32_t meshx_net_key_update(uint16_t net_key_index, meshx_net_key_t net_key);
MESHX_EXTERN int32_t meshx_net_key_delete(uint16_t net_key_index);
MESHX_EXTERN void meshx_net_key_clear(void);

MESHX_END_DECLS

#endif

