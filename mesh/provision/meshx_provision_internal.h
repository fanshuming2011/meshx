/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef _MESHX_PROVISION_INTERNAL_H_
#define _MESHX_PROVISION_INTERNAL_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_provision_receive(meshx_bearer_t bearer, const uint8_t *pdata,
                                             uint8_t len);
MESHX_EXTERN int32_t meshx_provision_pdu_process(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid,
                                                 const uint8_t *pdata, uint8_t len);
MESHX_EXTERN void meshx_provision_state_changed(meshx_dev_uuid_t dev_uuid, uint8_t new_state,
                                                uint8_t old_state);

MESHX_END_DECLS

#endif