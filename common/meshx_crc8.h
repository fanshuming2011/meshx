/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_CRC8_H_
#define _MESHX_CRC8_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

uint8_t meshx_crc8(const uint8_t *pdata, uint32_t len);

MESHX_END_DECLS


#endif