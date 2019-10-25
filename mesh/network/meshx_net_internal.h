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
#include "meshx_list.h"

MESHX_BEGIN_DECLS


typedef struct
{
    uint8_t type;
    meshx_bearer_t bearer;
    meshx_net_iface_ifilter_t ifilter;
    meshx_net_iface_ofilter_t ofilter;
    meshx_net_iface_filter_info_t filter_info;
    meshx_list_t node;
} meshx_net_iface_info_t;

extern meshx_list_t meshx_net_iface_list;

MESHX_EXTERN int32_t meshx_net_iface_init(void);
MESHX_EXTERN bool meshx_net_iface_ifilter(meshx_net_iface_t net_iface,
                                          const meshx_net_iface_ifilter_data_t *pdata);
MESHX_EXTERN bool meshx_net_iface_ofilter(meshx_net_iface_t net_iface,
                                          const meshx_net_iface_ofilter_data_t *pdata);
meshx_bearer_t meshx_net_iface_get_bearer(meshx_net_iface_t net_iface);

MESHX_END_DECLS

#endif /* _MESHX_NET_INTERNAL_H_ */
