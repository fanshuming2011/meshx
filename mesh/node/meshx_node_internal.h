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
    meshx_node_config_t config;
    meshx_node_param_t param;
} meshx_node_params_t;

MESHX_EXTERN meshx_node_params_t meshx_node_params;

MESHX_END_DECLS

#endif /* _MESHX_NODE_INTERNAL_H_ */
