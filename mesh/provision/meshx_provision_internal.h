/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef _MESHX_PROVISION_INTERNAL_H_
#define _MESHX_PROVISION_INTERNAL_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS

int32_t meshx_provision_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len);

MESHX_END_DECLS

#endif