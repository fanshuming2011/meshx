/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NETWORK_H_
#define _MESHX_NETWORK_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS



#define MESHX_NETWORK_IF_TYPE_INVALID           0
#define MESHX_NETWORK_IF_TYPE_ADV               1
#define MESHX_NETWORK_IF_TYPE_GATT              2
#define MESHX_NETWORK_IF_TYPE_LOOPBACK          3

typedef void *meshx_network_if_t;

typedef struct
{
} meshx_network_if_input_filter_data_t;

typedef struct
{
    uint16_t src_addr;
    uint16_t dst_addr;
} meshx_network_if_output_filter_data_t;

typedef struct
{
    uint32_t total_receive;
    uint32_t filtered_receive;
    uint32_t total_send;
    uint32_t filtered_send;
} meshx_network_if_filter_info_t;;


/**
 * TRUE: pass filter
 * FALSE: failed to pass filter
 */
typedef bool (*meshx_network_if_input_filter_t)(meshx_network_if_t network_if,
                                                const meshx_network_if_input_filter_data_t *pdata);
typedef bool (*meshx_network_if_output_filter_t)(meshx_network_if_t network_if,
                                                 const meshx_network_if_output_filter_data_t *pdata);


MESHX_EXTERN int32_t meshx_network_init(void);

MESHX_EXTERN meshx_network_if_t meshx_network_if_create(void);
MESHX_EXTERN void meshx_network_if_delete(meshx_network_if_t network_if);
MESHX_EXTERN int32_t meshx_network_if_connect(meshx_network_if_t network_if, meshx_bearer_t bearer,
                                              meshx_network_if_input_filter_t in_filter, meshx_network_if_output_filter_t out_filter);
MESHX_EXTERN void meshx_network_if_disconnect(meshx_network_if_t network_if);
MESHX_EXTERN meshx_network_if_t meshx_network_if_get(meshx_bearer_t bearer);

MESHX_EXTERN int32_t meshx_network_receive(meshx_network_if_t network_if, const uint8_t *pdata,
                                           uint8_t len);
MESHX_EXTERN int32_t meshx_network_send(meshx_network_if_t network_if,
                                        const uint8_t *ppdu, uint8_t pdu_len,
                                        const meshx_msg_ctx_t *pmsg_ctx);

MESHX_EXTERN meshx_network_if_filter_info_t meshx_network_if_get_filter_info(
    meshx_network_if_t network_if);

MESHX_END_DECLS

#endif /* _MESHX_NETWORK_H_ */
