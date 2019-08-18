/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_CONFIG_H_
#define _MESHX_CONFIG_H_

#define MESHX_SUPPORT_ROLE_DEVICE                  1
#define MESHX_SUPPORT_ROLE_PROVISIONER             1

#define MESHX_SUPPORT_ADV_BEARER                   1
#define MESHX_SUPPORT_GATT_BEARER                  1

#define MESHX_TRACE_FAST                           0
#define MESHX_ENABLE_ASSERT                        1

#define MESHX_USE_SAMPLE_DATA                      1

#define MESHX_GAP_SCAN_WINDOW                      160 /* 100ms */
#define MESHX_GAP_SCAN_INTERVAL                    160 /* 100ms */

/* network interface */
#define MESHX_NETWORK_IF_MAX_NUM                   9





#endif /* _MESHX_CONFIG_H_ */