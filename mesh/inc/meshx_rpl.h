/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_RPL_H_
#define _MESHX_RPLH_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef struct
{
    uint16_t src;
    uint32_t seq;
    uint32_t iv_index;
} meshx_rpl_t;

MESHX_EXTERN int32_t meshx_rpl_init(void);
MESHX_EXTERN void meshx_rpl_deinit(void);
MESHX_EXTERN int32_t meshx_rpl_update(meshx_rpl_t rpl);
MESHX_EXTERN void meshx_rpl_clear(void);
MESHX_EXTERN bool meshx_rpl_check(meshx_rpl_t rpl);
MESHX_EXTERN bool meshx_rpl_is_full(void);

MESHX_END_DECLS



#endif /* _MESHX_RPL_H_ */
