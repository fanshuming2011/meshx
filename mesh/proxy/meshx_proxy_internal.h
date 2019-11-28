/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_PROXY_INTERNAL_H_
#define _MESHX_PROXY_INTERNAL_H_

#include "meshx_async_internal.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN void meshx_proxy_async_handle_sar_timeout(meshx_async_msg_t msg);
MESHX_EXTERN uint16_t meshx_proxy_char_data_out_handle(void);
MESHX_EXTERN uint16_t meshx_proxy_char_data_in_handle(void);
MESHX_EXTERN uint16_t meshx_prov_char_data_out_handle(void);
MESHX_EXTERN uint16_t meshx_prov_char_data_in_handle(void);
MESHX_EXTERN int32_t meshx_prov_server_receive(meshx_bearer_t bearer, const uint8_t *pdata,
                                               uint16_t len);
MESHX_EXTERN int32_t meshx_proxy_server_receive(meshx_bearer_t bearer, const uint8_t *pdata,
                                                uint16_t len);

MESHX_END_DECLS

#endif /* _MESHX_PROXY_INTERNAL_H_ */
