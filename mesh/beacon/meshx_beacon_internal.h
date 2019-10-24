/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_BEACON_INTERNAL_H_
#define _MESHX_BEACON_INTERNAL_H_

#include "meshx_async_internal.h"
#include "meshx_common.h"

MESHX_BEGIN_DECLS

typedef struct
{
    uint8_t key_refresh: 1;
    uint8_t iv_update: 1;
    uint8_t rsvd: 6;
} __PACKED meshx_snb_flag_t;;


typedef struct
{
    uint8_t type;
    meshx_snb_flag_t flag;
    uint8_t net_id[8];
    uint32_t iv_index;
    uint8_t auth_valud[8];
} __PACKED meshx_snb_t;;


MESHX_EXTERN void meshx_beacon_async_handle_timeout(meshx_async_msg_t msg);
MESHX_EXTERN int32_t meshx_beacon_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len,
                                          const meshx_bearer_rx_metadata_adv_t *padv_metadata);

MESHX_END_DECLS

#endif /* _MESHX_BEACON_INTERNAL_H_ */
