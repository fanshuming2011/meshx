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

MESHX_BEGIN_DECLS

typedef enum
{
    MESHX_PROVISION_LINK_CLOSE_SUCCESS,
    MESHX_PROVISION_LINK_CLOSE_TIMEOUT,
    MESHX_PROVISION_LINK_CLOSE_FAIL,
    MESHX_PROVISION_LINK_CLOSE_LINK_LOSS,
} meshx_provision_link_close_t;

typedef enum
{
    MESHX_PROVISION_LINK_OPEN_SUCCESS,
    MESHX_PROVISION_LINK_OPEN_TIMEOUT,
} meshx_provision_link_open_t;

#define MESHX_PROVISION_CB_TYPE_LINK_OPEN            0x00 /* @ref meshx_provision_link_open_t */
#define MESHX_PROVISION_CB_TYPE_LINK_CLOSE           0x01 /* @ref meshx_provision_link_close_t */
#define MESHX_PROVISION_CB_TYPE_GET_INVITE           0x02 /* @ref meshx_provision_invite_t */
#define MESHX_PROVISION_CB_TYPE_SET_INVITE           0x03 /* @ref meshx_provision_invite_t */
#define MESHX_PROVISION_CB_TYPE_FAILED               0x04 /* @ref meshx provisison failed error code macros */
#define MESHX_PROVISION_CB_TYPE_COMPLETE             0x05 /* @ref NULL */


MESHX_END_DECLS

#endif /* _MESHX_PROVISION_CALLBACK_H_ */
