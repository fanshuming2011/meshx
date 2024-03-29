/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_ASYNC_INTERNAL_H_
#define _MESHX_ASYNC_INTERNAL_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_PB_ADV                            0
#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_BEACON                            1
#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_TX                    2
#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_RX_ACK                3
#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_RX_INCOMPLETE         4
#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_IV_INDEX                          5
#define MESHX_ASYNC_MSG_TYPE_TIMEOUT_PROXY_SAR                         6

typedef struct
{
    uint8_t type;
    union
    {
        uint32_t data;
        void *pdata;
    };
    uint8_t data_len;
} meshx_async_msg_t;

MESHX_EXTERN int32_t meshx_async_msg_send(const meshx_async_msg_t *pmsg);

MESHX_END_DECLS


#endif /* _MESHX_ASYNC_INTERNAL_H_ */
