/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NET_H_
#define _MESHX_NET_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS



#define MESHX_NET_IFACE_TYPE_INVALID           0
#define MESHX_NET_IFACE_TYPE_ADV               1
#define MESHX_NET_IFACE_TYPE_GATT              2
#define MESHX_NET_IFACE_TYPE_LOOPBACK          3

typedef void *meshx_net_iface_t;

typedef struct
{
} meshx_net_iface_ifilter_data_t;

typedef struct
{
    uint16_t src_addr;
    uint16_t dst_addr;
} meshx_net_iface_ofilter_data_t;

typedef struct
{
    uint32_t total_receive;
    uint32_t filtered_receive;
    uint32_t total_send;
    uint32_t filtered_send;
} meshx_net_iface_filter_info_t;;


/**
 * TRUE: pass filter
 * FALSE: failed to pass filter
 */
typedef bool (*meshx_net_iface_ifilter_t)(meshx_net_iface_t net_iface,
                                          const meshx_net_iface_ifilter_data_t *pdata);
typedef bool (*meshx_net_iface_ofilter_t)(meshx_net_iface_t net_iface,
                                          const meshx_net_iface_ofilter_data_t *pdata);


MESHX_EXTERN int32_t meshx_net_init(void);

MESHX_EXTERN meshx_net_iface_t meshx_net_iface_create(void);
MESHX_EXTERN void meshx_net_iface_delete(meshx_net_iface_t net_iface);
MESHX_EXTERN int32_t meshx_net_iface_connect(meshx_net_iface_t net_iface, meshx_bearer_t bearer,
                                             meshx_net_iface_ifilter_t ifilter, meshx_net_iface_ofilter_t ofilter);
MESHX_EXTERN void meshx_net_iface_disconnect(meshx_net_iface_t net_iface);
MESHX_EXTERN bool meshx_net_iface_is_connect(meshx_net_iface_t net_iface);
MESHX_EXTERN meshx_net_iface_t meshx_net_iface_get(meshx_bearer_t bearer);
MESHX_EXTERN uint8_t meshx_net_iface_type_get(meshx_net_iface_t net_iface);

MESHX_EXTERN int32_t meshx_net_receive(meshx_net_iface_t net_iface, const uint8_t *pdata,
                                       uint8_t len);
MESHX_EXTERN int32_t meshx_net_send(meshx_net_iface_t net_iface,
                                    const uint8_t *ptrans_pdu, uint8_t trans_pdu_len,
                                    const meshx_msg_ctx_t *pmsg_ctx);

MESHX_EXTERN meshx_net_iface_filter_info_t meshx_net_iface_get_filter_info(
    meshx_net_iface_t net_iface);

MESHX_END_DECLS

#endif /* _MESHX_NET_H_ */
