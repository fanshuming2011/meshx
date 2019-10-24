/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_UPPER_TRANS_H_
#define _MESHX_UPPER_TRANS_H_

#include "meshx_net.h"

MESHX_BEGIN_DECLS


MESHX_EXTERN int32_t meshx_upper_trans_init(void);
MESHX_EXTERN int32_t meshx_upper_trans_send(meshx_net_iface_t net_iface,
                                            const uint8_t *pdata, uint16_t len,
                                            meshx_msg_ctx_t *pmsg_tx_ctx);
MESHX_EXTERN int32_t meshx_upper_trans_receive(meshx_net_iface_t net_iface,
                                               uint8_t *pdata,
                                               uint8_t len, meshx_msg_ctx_t *pmsg_rx_ctx);

MESHX_END_DECLS

#endif /* _MESHX_UPPER_TRANS_H_ */