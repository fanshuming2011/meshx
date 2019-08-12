/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NETWORK_INTERNAL_H_
#define _MESHX_NETWORK_INTERNAL_H_

#include "meshx_common.h"
#include "meshx_network.h"
#include "meshx_bearer.h"

MESHX_BEGIN_DECLS



MESHX_EXTERN int32_t meshx_network_if_init(void);
MESHX_EXTERN bool meshx_network_if_input_filter(meshx_network_if_t network_if,
                                                const meshx_network_if_filter_data_t *pdata);
MESHX_EXTERN bool meshx_network_if_output_filter(meshx_network_if_t network_if,
                                                 const meshx_network_if_filter_data_t *pdata);
meshx_bearer_t meshx_network_if_get_bearer(meshx_network_if_t network_if);

MESHX_END_DECLS

#endif /* _MESHX_NETWORK_INTERNAL_H_ */