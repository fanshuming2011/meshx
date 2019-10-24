/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NET_INTERNAL_H_
#define _MESHX_NET_INTERNAL_H_

#include "meshx_common.h"
#include "meshx_net.h"
#include "meshx_bearer.h"

MESHX_BEGIN_DECLS



MESHX_EXTERN int32_t meshx_net_iface_init(void);
MESHX_EXTERN bool meshx_net_iface_ifilter(meshx_net_iface_t net_iface,
                                          const meshx_net_iface_ifilter_data_t *pdata);
MESHX_EXTERN bool meshx_net_iface_ofilter(meshx_net_iface_t net_iface,
                                          const meshx_net_iface_ofilter_data_t *pdata);
meshx_bearer_t meshx_net_iface_get_bearer(meshx_net_iface_t net_iface);

MESHX_END_DECLS

#endif /* _MESHX_NET_INTERNAL_H_ */
