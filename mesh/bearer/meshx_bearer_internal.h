/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_BEARER_INTERNAL_H_
#define _MESHX_BEARER_INTERNAL_H_

#include "meshx_types.h"
#include "meshx_errno.h"
#include "meshx_bearer.h"
#include "meshx_net.h"

MESHX_BEGIN_DECLS

struct _meshx_bearer
{
    uint8_t type;
    meshx_net_iface_t net_iface;
};


MESHX_EXTERN int32_t meshx_bearer_adv_init(void);
MESHX_EXTERN void meshx_bearer_adv_delete(meshx_bearer_t bearer);

MESHX_EXTERN int32_t meshx_bearer_gatt_init(void);
MESHX_EXTERN void meshx_bearer_gatt_delete(meshx_bearer_t bearer);
MESHX_EXTERN uint16_t meshx_bearer_gatt_mtu_get(meshx_bearer_t bearer);



MESHX_END_DECLS

#endif /* _MESHX_BEARER_INTERNAL_H_ */
