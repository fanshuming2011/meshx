/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_PROXY_H_
#define _MESHX_PROXY_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS

#define MESHX_PROXY_MSG_TYPE_NET                0
#define MESHX_PROXY_MSG_TYPE_BEACON             1
#define MESHX_PROXY_MSG_TYPE_PROXY_CFG          2
#define MESHX_PROXY_MSG_TYPE_PROV               3

int32_t meshx_proxy_init(void);
int32_t meshx_proxy_send(meshx_bearer_t bearer, uint8_t msg_type, const uint8_t *pdata,
                         uint16_t len);
int32_t meshx_proxy_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint16_t len);

MESHX_END_DECLS

#endif /* _MESHX_PROXY_H_ */
