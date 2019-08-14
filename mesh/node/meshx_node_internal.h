/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NODE_INTERNAL_H_
#define _MESHX_NODE_INTERNAL_H_

#include "meshx_node.h"

MESHX_BEGIN_DECLS

typedef struct
{
    meshx_dev_uuid_t dev_uuid;
    uint16_t node_addr;
    uint32_t udb_interval;
    uint32_t snb_interval;
    meshx_key_t dev_key;
    uint16_t net_key_num;
    uint16_t app_key_num;
    uint16_t nmc_size;
    uint16_t rpl_size;
} meshx_node_params_t;

MESHX_EXTERN meshx_node_params_t meshx_node_params;

MESHX_END_DECLS

#endif /* _MESHX_NODE_INTERNAL_H_ */
