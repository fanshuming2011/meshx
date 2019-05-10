/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_PROVISION_H_
#define _MESHX_PROVISION_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS


MESHX_EXTERN int32_t meshx_provision_init(void);

MESHX_EXTERN int32_t meshx_provision_link_open(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid);
MESHX_EXTERN int32_t meshx_provision_link_ack(meshx_bearer_t bearer);
MESHX_EXTERN int32_t meshx_provision_link_close(meshx_bearer_t bearer, uint8_t reason);

MESHX_EXTERN int32_t meshx_provision_send(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len);
MESHX_EXTERN int32_t meshx_provision_receive(meshx_bearer_t bearer, const uint8_t *pdata,
                                             uint8_t len);

MESHX_END_DECLS


#endif /* _MESHX_PROVISION_H_ */
