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
#include "meshx_provision.h"
#include "meshx_async_internal.h"
#include "meshx_notify.h"

MESHX_BEGIN_DECLS

struct _meshx_provision_dev
{
    meshx_dev_uuid_t dev_uuid;
    meshx_bearer_t bearer;
    meshx_provision_state_t state;
};


MESHX_EXTERN int32_t meshx_provision_receive(meshx_bearer_t bearer, const uint8_t *pdata,
                                             uint8_t len);
MESHX_EXTERN int32_t meshx_provision_pdu_process(meshx_provision_dev_t prov_dev,
                                                 const uint8_t *pdata, uint8_t len);

MESHX_EXTERN void meshx_pb_adv_async_handle_timeout(meshx_async_msg_t msg);

MESHX_EXTERN int32_t meshx_provision_handle_notify(meshx_bearer_t bearer,
                                                   const meshx_notify_prov_t *pnotify, uint8_t len);

MESHX_END_DECLS

#endif