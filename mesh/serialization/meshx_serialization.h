/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_SERIALIZATION_H_
#define _MESHX_SERIALIZATION_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef struct
{
    uint8_t dev_key[16];
} meshx_node_layout_t;


MESHX_EXTERN int32_t meshx_serialize_init(void);
MESHX_EXTERN int32_t meshx_serialize_set_value(uint32_t id, const uint8_t *pvalue,
                                               uint32_t value_len);

MESHX_END_DECLS


#endif /* _MESHX_SERIALIZATION_H_ */

