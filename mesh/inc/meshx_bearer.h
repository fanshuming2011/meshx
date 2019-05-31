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

#define MESHX_BEARER_PKT_TYPE_UNKNOWN                 0xff

#define MESHX_BEARER_ADV_PKT_TYPE_PB_ADV               MESHX_GAP_ADTYPE_PB_ADV
#define MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG             MESHX_GAP_ADTYPE_MESH_MSG
#define MESHX_BEARER_ADV_PKT_TYPE_BEACON               MESHX_GAP_ADTYPE_MESH_BEACON
#define MESHX_IS_BEARER_ADV_PKT_TYPE_VALID(type)       ((type == MESHX_BEARER_ADV_PKT_TYPE_PB_ADV) || \
                                                        (type == MESHX_BEARER_ADV_PKT_TYPE_MESH_MSG) || \
                                                        (type == MESHX_BEARER_ADV_PKT_TYPE_BEACON))

#define MESHX_BEARER_GATT_PKT_TYPE_NETWORK              0x00
#define MESHX_BEARER_GATT_PKT_TYPE_BEACON               0x01
#define MESHX_BEARER_GATT_PKT_TYPE_PROXY_CONFIG         0x02
#define MESHX_BEARER_GATT_PKT_TYPE_PROVISION            0x03
#define MESHX_IS_BEARER_GATT_ADV_PKT_TYPE_VALID(type)   ((type == MESHX_BEARER_GATT_PKT_TYPE_NETWORK) || \
                                                         (type == MESHX_BEARER_GATT_PKT_TYPE_BEACON) || \
                                                         (type == MESHX_BEARER_GATT_PKT_TYPE_PROXY_CONFIG) || \
                                                         (type == MESHX_BEARER_GATT_PKT_TYPE_PROVISION))


#define MESHX_INVALID_BEARER           0

typedef struct
{
    /**
     * >=0 means application will not get message when advertising finished,
     *     need to wait adv_period(unit is ms) for advertising finished.
     * <0 means application will get message when advertising finished
     */
    int8_t adv_period;
} meshx_bearer_param_adv_t;

typedef struct
{
    uint16_t conn_id;
} meshx_bearer_param_gatt_t;

typedef struct
{
    uint8_t bearer_type;
    union
    {
        meshx_bearer_param_adv_t param_adv;
        meshx_bearer_param_gatt_t param_gatt;
    };
} meshx_bearer_param_t;

typedef struct _meshx_bearer *meshx_bearer_t;

MESHX_EXTERN int32_t meshx_bearer_init(void);
MESHX_EXTERN meshx_bearer_t meshx_bearer_create(meshx_bearer_param_t bearer_param);
MESHX_EXTERN void meshx_bearer_delete(meshx_bearer_t bearer);
MESHX_EXTERN uint8_t meshx_bearer_type_get(meshx_bearer_t bearer);

MESHX_EXTERN int32_t meshx_bearer_send(meshx_bearer_t bearer, uint8_t pkt_type,
                                       const uint8_t *pdata, uint8_t len);
MESHX_EXTERN int32_t meshx_bearer_receive(const uint8_t *pdata, uint8_t len,
                                          const meshx_bearer_rx_metadata_t *prx_metadata);

MESHX_END_DECLS

#endif /* _MESHX_BEARER_H_ */

