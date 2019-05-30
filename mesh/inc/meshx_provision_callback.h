/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_PROVISION_CALLBACK_H_
#define _MESHX_PROVISION_CALLBACK_H_

#include "meshx_types.h"
#include "meshx_gap.h"

MESHX_BEGIN_DECLS

#define MESHX_PROVISION_CB_TYPE_STATE_CHANGED        0x00 /* @ref meshx_provision_state_changed_t*/
#define MESHX_PROVISION_CB_TYPE_GET_INVITE           0x01 /* @ref meshx_provision_invite_t */
#define MESHX_PROVISION_CB_TYPE_SET_INVITE           0x02 /* @ref meshx_provision_invite_t */
typedef struct
{
    meshx_dev_uuid_t uuid;
    uint8_t new_state;
    uint8_t old_state;
} meshx_provision_state_changed_t;


#define MESHX_PROVISION_STATE_IDLE                   0
#define MESHX_PROVISION_STATE_LINK_OPENING           0x01
#define MESHX_PROVISION_STATE_LINK_OPENED            0x02
#define MESHX_PROVISION_STATE_INVITE                 0x03
#define MESHX_PROVISION_STATE_CAPABILITES            0x04

typedef int32_t (*meshx_provision_callback_t)(uint8_t type, void *pargs);

MESHX_END_DECLS

#endif /* _MESHX_PROVISION_CALLBACK_H_ */
