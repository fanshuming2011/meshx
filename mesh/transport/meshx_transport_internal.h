/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_TRANSPORT_INTERNAL_H_
#define _MESHX_TRANSPORT_INTERNAL_H_

#include "meshx_async_internal.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN void meshx_lower_trans_async_handle_tx_timeout(meshx_async_msg_t msg);
MESHX_EXTERN void meshx_lower_trans_async_handle_rx_ack_timeout(meshx_async_msg_t msg);
MESHX_EXTERN void meshx_lower_trans_async_handle_rx_incomplete_timeout(meshx_async_msg_t msg);

MESHX_END_DECLS


#endif /* _MESHX_TRANSPORT_INTERNAL_H_ */
