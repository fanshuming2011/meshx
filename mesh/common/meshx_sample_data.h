/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_SAMPLE_DATA_H_
#define _MESHX_SAMPLE_DATA_H_

#include "meshx_prov.h"
#include "meshx_common.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN const uint8_t sample_prov_public_key[64];
MESHX_EXTERN const uint8_t sample_prov_private_key[32];
MESHX_EXTERN const uint8_t sample_prov_random[16];
MESHX_EXTERN const uint8_t sample_device_public_key[64];
MESHX_EXTERN const uint8_t sample_device_private_key[32];
MESHX_EXTERN const uint8_t sample_device_random[16];
MESHX_EXTERN const meshx_prov_capabilites_t sample_prov_capabilites;
MESHX_EXTERN const meshx_prov_start_t sample_prov_start;
MESHX_EXTERN const meshx_prov_data_t sample_prov_data;
MESHX_EXTERN meshx_key_t sample_net_key;
MESHX_EXTERN meshx_key_t sample_net_key1;
MESHX_EXTERN meshx_key_t sample_app_key;
MESHX_EXTERN meshx_key_t sample_dev_key;


MESHX_END_DECLS

#endif
