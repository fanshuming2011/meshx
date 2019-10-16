/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_UPPER_TRANSPORT_H_
#define _MESHX_UPPER_TRANSPORT_H_

#include "meshx_network.h"

MESHX_BEGIN_DECLS


MESHX_EXTERN int32_t meshx_upper_transport_init(void);
MESHX_EXTERN int32_t meshx_upper_transport_send(meshx_network_if_t network_if,
                                                const uint8_t *pdata, uint16_t len,
                                                meshx_msg_ctx_t *pmsg_ctx);
MESHX_EXTERN int32_t meshx_upper_transport_receive(meshx_network_if_t network_if,
                                                   const uint8_t *pdata,
                                                   uint8_t len, meshx_msg_ctx_t *pmsg_ctx);

MESHX_END_DECLS

#endif /* _MESHX_UPPER_TRANSPORT_H_ */