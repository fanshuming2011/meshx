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
MESHX_EXTERN const meshx_application_key_t *meshx_app_key_get(uint16_t app_key_index);
MESHX_EXTERN int32_t meshx_app_key_add(uint16_t net_key_index, uint16_t app_key_index,
                                       meshx_key_t app_key);
MESHX_EXTERN int32_t meshx_app_key_update(uint16_t net_key_index, uint16_t app_key_index,
                                          meshx_key_t app_key);
MESHX_EXTERN int32_t meshx_app_key_delte(uint16_t net_key_index, uint16_t app_key_index);
MESHX_EXTERN void meshx_app_key_clear(void);

MESHX_EXTERN int32_t meshx_net_key_init(void);
MESHX_EXTERN const meshx_network_key_t *meshx_net_key_get(uint16_t net_key_index);
MESHX_EXTERN const meshx_network_key_t *meshx_net_key_get_by_nid(uint8_t nid);
MESHX_EXTERN int32_t meshx_net_key_add(uint16_t net_key_index, meshx_key_t net_key);
MESHX_EXTERN int32_t meshx_net_key_update(uint16_t net_key_index, meshx_key_t net_key);
MESHX_EXTERN int32_t meshx_net_key_delete(uint16_t net_key_index);
MESHX_EXTERN void meshx_net_key_clear(void);

MESHX_EXTERN const meshx_key_t *meshx_dev_key_get(void);
MESHX_EXTERN int32_t meshx_dev_key_set(meshx_key_t dev_key);


MESHX_END_DECLS



#endif /* _MESHX_KEY_H_ */