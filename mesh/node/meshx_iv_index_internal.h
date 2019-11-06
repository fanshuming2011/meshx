/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_IV_INDEX_INTERNAL_H_
#define _MESHX_IV_INDEX_INTERNAL_H_

#include "meshx_async_internal.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN void meshx_iv_index_async_handle_timeout(meshx_async_msg_t msg);
MESHX_EXTERN bool meshx_is_iv_update_state_transit_pending(void);

MESHX_END_DECLS


#endif
