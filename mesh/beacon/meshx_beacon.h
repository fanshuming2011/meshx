/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_BEACON_H_
#define _MESHX_BEACON_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef enum
{
    MESHX_BEACON_TYPE_UDB,
    MESHX_BEACON_TYPE_SNB,
} meshx_beacon_type_t;

MESHX_EXTERN int32_t meshx_beacon_start(meshx_beacon_type_t type, uint32_t interval);
MESHX_EXTERN void meshx_beacon_stop(meshx_beacon_type_t type);


MESHX_END_DECLS


#endif /* _MESHX_BEACON_H */
