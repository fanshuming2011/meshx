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

MESHX_EXTERN void meshx_set_device_uuid(const meshx_dev_uuid_t uuid);
MESHX_EXTERN void meshx_get_device_uuid(meshx_dev_uuid_t uuid);
MESHX_EXTERN uint16_t meshx_get_node_address(void);

MESHX_END_DECLS

#endif

