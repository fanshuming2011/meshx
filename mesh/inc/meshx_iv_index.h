/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_IV_INDEX_H_
#define _MESHX_IV_INDEX_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef enum
{
    MESHX_IV_UPDATE_STATE_NORMAL,
    MESHX_IV_UPDATE_STATE_IN_PROGRESS,
} meshx_iv_update_state_t;

MESHX_EXTERN int32_t meshx_iv_index_init(void);
MESHX_EXTERN void meshx_iv_index_deinit(void);
MESHX_EXTERN uint32_t meshx_iv_index_get(void);
MESHX_EXTERN uint32_t meshx_tx_iv_index_get(void);
MESHX_EXTERN void meshx_iv_index_set(uint32_t iv_index);
MESHX_EXTERN int32_t meshx_iv_index_update(uint32_t iv_index, meshx_iv_update_state_t state);
MESHX_EXTERN int32_t meshx_iv_update_state_transit(meshx_iv_update_state_t state);
MESHX_EXTERN meshx_iv_update_state_t meshx_iv_update_state_get(void);
MESHX_EXTERN void meshx_iv_update_operate_time_set(uint32_t time);
MESHX_EXTERN void meshx_iv_test_mode_enable(bool flag);



MESHX_END_DECLS


#endif
