/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NOTIFY_INTERNAL_H_
#define _MESHX_NOTIFY_INTERNAL_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_notify(meshx_bearer_t bearer, uint8_t notify_type, const void *pdata,
                                  uint8_t len);

MESHX_END_DECLS

#endif /* _MESHX_NOTIFY_INTERNAL_H_ */
