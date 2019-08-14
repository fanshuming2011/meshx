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
#define MESHX_SEQ_INVALID          0xFFFFFFFF

MESHX_EXTERN int32_t meshx_seq_init(uint8_t element_num);
MESHX_EXTERN void meshx_seq_deinit(void);
MESHX_EXTERN uint32_t meshx_seq_get(uint8_t element_index);
MESHX_EXTERN uint32_t meshx_seq_set(uint8_t element_index, uint32_t seq);
MESHX_EXTERN uint32_t meshx_seq_use(uint8_t element_index);

MESHX_END_DECLS

#endif
