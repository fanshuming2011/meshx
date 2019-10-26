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

/* parameters can't be changed in runtime */
typedef struct
{
    meshx_role_t role;
    meshx_dev_uuid_t dev_uuid;
    bool adv_bearer_enable;
    bool gatt_bearer_enable;
    uint16_t net_key_num;
    uint16_t app_key_num;
    uint16_t dev_key_num;
    uint16_t nmc_size;
    uint16_t rpl_size;
    uint8_t gap_task_num;
    uint8_t trans_tx_task_num;
    uint8_t trans_rx_task_num;
    uint8_t trans_tx_retry_times;
} meshx_node_config_t;

/* parameters can be changed in runtime */
typedef struct
{
    uint16_t node_addr;
    uint32_t udb_interval; /* unit is 100ms */
    uint32_t snb_interval; /* unit is 100ms */
    uint8_t trans_retrans_count;
    uint8_t default_ttl;
} meshx_node_param_t;

typedef enum
{
    MESHX_NODE_PARAM_TYPE_NODE_ADDR,
    MESHX_NODE_PARAM_TYPE_UDB_INTERVAL,
    MESHX_NODE_PARAM_TYPE_SNB_INTERVAL,
    MESHX_NODE_PARAM_TYPE_TRANS_RETRANS_COUNT,
    MESHX_NODE_PARAM_TYPE_DEFAULT_TTL,
} meshx_node_param_type_t;


MESHX_EXTERN bool meshx_node_is_my_address(uint16_t addr);
MESHX_EXTERN bool meshx_node_is_accept_address(uint16_t addr);

MESHX_EXTERN int32_t meshx_node_config_init(meshx_node_config_t *pconfig);
MESHX_EXTERN int32_t meshx_node_config_set(const meshx_node_config_t *pconfig);
MESHX_EXTERN meshx_node_config_t meshx_node_config_get(void);

MESHX_EXTERN int32_t meshx_node_params_init(meshx_node_param_t *pparam);
MESHX_EXTERN int32_t meshx_node_params_set(const meshx_node_param_t *pparam);
MESHX_EXTERN meshx_node_param_t meshx_node_params_get(void);

MESHX_EXTERN int32_t meshx_node_param_set(meshx_node_param_type_t type, const void *pdata);
MESHX_EXTERN int32_t meshx_node_param_get(meshx_node_param_type_t type, void *pdata);

MESHX_END_DECLS

#endif

