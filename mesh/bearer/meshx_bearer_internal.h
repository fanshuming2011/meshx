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
MESHX_EXTERN meshx_bearer_t meshx_bearer_adv_create(meshx_bearer_param_adv_t adv_param);
MESHX_EXTERN void meshx_bearer_adv_delete(meshx_bearer_t bearer);
MESHX_EXTERN int32_t meshx_bearer_adv_send(meshx_bearer_t bearer, uint8_t pkt_type,
                                           const uint8_t *pdata, uint8_t len);

MESHX_EXTERN int32_t meshx_bearer_adv_receive(meshx_bearer_t bearer, uint8_t adv_type,
                                              const uint8_t *pdata,
                                              uint8_t len, const meshx_bearer_rx_metadata_adv_t *padv_metadata);
MESHX_EXTERN meshx_bearer_t meshx_bearer_adv_get(void);

MESHX_EXTERN int32_t meshx_bearer_gatt_init(void);
MESHX_EXTERN meshx_bearer_t meshx_bearer_gatt_create(meshx_bearer_param_gatt_t gatt_param);
MESHX_EXTERN void meshx_bearer_gatt_delete(meshx_bearer_t bearer);
MESHX_EXTERN int32_t meshx_bearer_gatt_send(meshx_bearer_t bearer, uint8_t pkt_type,
                                            const uint8_t *pdata,
                                            uint8_t len);
MESHX_EXTERN int32_t meshx_bearer_gatt_receive(meshx_bearer_t bearer, const uint8_t *pdata,
                                               uint8_t len);
MESHX_EXTERN meshx_bearer_t meshx_bearer_gatt_get(const meshx_bearer_rx_metadata_gatt_t
                                                  *prx_metadata);



MESHX_END_DECLS

#endif /* _MESHX_BEARER_INTERNAL_H_ */
