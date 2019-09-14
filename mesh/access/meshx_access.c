/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_ACCESS"
#include "meshx_access.h"
#include "meshx_errno.h"
#include "meshx_trace.h"
#include "meshx_assert.h"

int32_t meshx_access_init(void)
{
    return MESHX_SUCCESS;
}

int32_t meshx_access_send(meshx_network_if_t network_if,
                          const uint8_t *pdata, uint16_t len, const meshx_access_msg_tx_ctx_t *pmsg_ctx)

{

}

