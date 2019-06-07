/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_ASYNC_INTERNAL_H_
#define _MESHX_ASYNC_INTERNAL_H_

#include "meshx_async.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN meshx_async_msg_notify_t meshx_async_msg_notify;

#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_PB_ADV_RETRY                      0
#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_PB_ADV_LINK_LOSS                  1

struct _meshx_async_msg
{
    uint8_t type;
    void *pdata;
    uint8_t data_len;
};

MESHX_END_DECLS


#endif /* _MESHX_ASYNC_INTERNAL_H_ */
