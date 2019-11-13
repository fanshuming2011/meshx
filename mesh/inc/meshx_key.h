/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_KEY_H_
#define _MESHX_KEY_H_

#include "meshx_common.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_app_key_init(void);
MESHX_EXTERN const meshx_app_key_t *meshx_app_key_get(uint16_t app_key_index);
MESHX_EXTERN const meshx_app_key_value_t *meshx_app_key_tx_get(uint16_t app_key_index);
MESHX_EXTERN void meshx_app_key_traverse_start(meshx_app_key_t **ptraverse_key);
MESHX_EXTERN void meshx_app_key_traverse_continue(meshx_app_key_t **ptraverse_key);
MESHX_EXTERN int32_t meshx_app_key_add(uint16_t net_key_index, uint16_t app_key_index,
                                       meshx_key_t app_key);
MESHX_EXTERN int32_t meshx_app_key_update(uint16_t net_key_index, uint16_t app_key_index,
                                          meshx_key_t app_key);
MESHX_EXTERN int32_t meshx_app_key_delte(uint16_t net_key_index, uint16_t app_key_index);
MESHX_EXTERN void meshx_app_key_clear(void);
MESHX_EXTERN int32_t meshx_app_key_state_transit(uint16_t net_key_index,
                                                 meshx_key_state_t key_state);

MESHX_EXTERN int32_t meshx_net_key_init(void);
MESHX_EXTERN const meshx_net_key_t *meshx_net_key_get(uint16_t net_key_index);
MESHX_EXTERN const meshx_net_key_value_t *meshx_net_key_tx_get(uint16_t net_key_index);
MESHX_EXTERN void meshx_net_key_traverse_start(meshx_net_key_t **ptraverse_key);
MESHX_EXTERN void meshx_net_key_traverse_continue(meshx_net_key_t **ptraverse_key);
MESHX_EXTERN int32_t meshx_net_key_add(uint16_t net_key_index, meshx_key_t net_key);
MESHX_EXTERN int32_t meshx_net_key_update(uint16_t net_key_index, meshx_key_t net_key);
MESHX_EXTERN int32_t meshx_net_key_delete(uint16_t net_key_index);
MESHX_EXTERN void meshx_net_key_clear(void);

MESHX_EXTERN int32_t meshx_dev_key_init(void);
MESHX_EXTERN const meshx_device_key_t *meshx_dev_key_get(uint16_t addr);
MESHX_EXTERN int32_t meshx_dev_key_add(uint16_t primary_addr, uint8_t element_num,
                                       meshx_key_t dev_key);
MESHX_EXTERN void meshx_dev_key_delete(meshx_key_t dev_key);
MESHX_EXTERN void meshx_dev_key_delete_by_addr(uint16_t addr);


MESHX_END_DECLS



#endif /* _MESHX_KEY_H_ */