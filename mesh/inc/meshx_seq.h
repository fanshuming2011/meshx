/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_SEQ_H_
#define _MESHX_SEQ_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

#define MESHX_SEQ_MAX              0xFFFFFF

MESHX_EXTERN uint32_t meshx_seq_get(void);
MESHX_EXTERN void meshx_seq_set(uint32_t seq);
MESHX_EXTERN void meshx_seq_increase(void);

MESHX_END_DECLS

#endif
