/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_BEACON_INTERNAL_H_
#define _MESHX_BEACON_INTERNAL_H_

#include "meshx_async_internal.h"
#include "meshx_common.h"

MESHX_BEGIN_DECLS

typedef struct
{
    uint8_t type;
    meshx_dev_uuid_t dev_uuid;
    uint16_t oob_info;
    uint32_t uri_hash;
} __PACKED meshx_udb_t;


MESHX_EXTERN void meshx_beacon_async_handle_timeout(meshx_async_msg_t msg);

MESHX_END_DECLS

#endif /* _MESHX_BEACON_INTERNAL_H_ */
