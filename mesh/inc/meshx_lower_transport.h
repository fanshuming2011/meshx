/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_LOWER_TRANSPORT_H_
#define _MESHX_LOWER_TRANSPORT_H_

#include "meshx_network.h"

MESHX_BEGIN_DECLS

typedef enum
{
    MESHX_LOWER_TRANSPORT_SEG_MSG_SEND_RESULT_SUCCESS,
    MESHX_LOWER_TRANSPORT_SEG_MSG_SEND_RESULT_NO_ACK,
    MESHX_LOWER_TRANSPORT_SEG_MSG_SEND_RESULT_CANCEL,
    MESHX_LOWER_TRANSPORT_SEG_MSG_SEND_RESULT_FAILED,
} meshx_lower_transport_seg_msg_send_result_t;

typedef void (*meshx_lower_transport_seg_msg_send_cb_t)(meshx_lower_transport_seg_msg_send_result_t
                                                        result);

MESHX_EXTERN int32_t meshx_lower_transport_init(void);
MESHX_EXTERN int32_t meshx_lower_transport_send(meshx_network_if_t network_if,
                                                const uint8_t *pupper_trans_pdu, uint16_t len,
                                                const meshx_msg_tx_ctx_t *pmsg_tx_ctx,
                                                meshx_lower_transport_seg_msg_send_cb_t send_cb);
MESHX_EXTERN int32_t meshx_lower_transport_receive(meshx_network_if_t network_if,
                                                   const uint8_t *pdata,
                                                   uint8_t len, meshx_msg_rx_ctx_t *pmsg_rx_ctx);


MESHX_END_DECLS

#endif /* _MESHX_LOWER_TRANSPORT_H_ */