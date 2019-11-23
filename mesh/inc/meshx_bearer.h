/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_BEARER_H_
#define _MESHX_BEARER_H_

#include "meshx_gap.h"

MESHX_BEGIN_DECLS

#define MESHX_BEARER_ADV_PKT_TYPE_PB_ADV                MESHX_GAP_ADTYPE_PB_ADV
#define MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG              MESHX_GAP_ADTYPE_MESH_MSG
#define MESHX_BEARER_ADV_PKT_TYPE_BEACON                MESHX_GAP_ADTYPE_MESH_BEACON
#define MESHX_IS_BEARER_ADV_PKT_TYPE_VALID(type)        ((type == MESHX_BEARER_ADV_PKT_TYPE_PB_ADV) || \
                                                         (type == MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG) || \
                                                         (type == MESHX_BEARER_ADV_PKT_TYPE_BEACON))

#define MESHX_BEARER_GATT_PKT_TYPE_NET                  0
#define MESHX_BEARER_GATT_PKY_TYPE_BEACON               1
#define MESHX_BEARER_GATT_PKT_TYPE_PROV                 3
#define MESHX_IS_BEARER_GATT_PKT_TYPE_VALID(type)       ((type == MESHX_BEARER_ADV_PKT_TYPE_PB_ADV) || \
                                                         (type == MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG) || \
                                                         (type == MESHX_BEARER_ADV_PKT_TYPE_BEACON))



typedef struct _meshx_bearer *meshx_bearer_t;

MESHX_EXTERN int32_t meshx_bearer_init(void);
MESHX_EXTERN meshx_bearer_t meshx_bearer_adv_create(void);
MESHX_EXTERN meshx_bearer_t meshx_bearer_gatt_create(uint8_t conn_id);
MESHX_EXTERN void meshx_bearer_delete(meshx_bearer_t bearer);
MESHX_EXTERN uint8_t meshx_bearer_type_get(meshx_bearer_t bearer);
MESHX_EXTERN meshx_bearer_t meshx_bearer_adv_get(void);
MESHX_EXTERN meshx_bearer_t meshx_bearer_gatt_get(uint8_t conn_id);

MESHX_EXTERN int32_t meshx_bearer_adv_send(meshx_bearer_t bearer, uint8_t pkt_type,
                                           const uint8_t *pdata, uint8_t len);

MESHX_EXTERN int32_t meshx_bearer_adv_receive(meshx_bearer_t bearer, uint8_t adv_type,
                                              const uint8_t *pdata,
                                              uint8_t len, const meshx_adv_metadata_t *padv_metadata);

MESHX_EXTERN int32_t meshx_bearer_gatt_send(meshx_bearer_t bearer, const uint8_t *pdata,
                                            uint8_t len);
MESHX_EXTERN int32_t meshx_bearer_gatt_receive(meshx_bearer_t bearer, const uint8_t *pdata,
                                               uint8_t len);

MESHX_END_DECLS

#endif /* _MESHX_BEARER_H_ */

