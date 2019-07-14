/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_SECURITY_H_
#define _MESHX_SECURITY_H_

#include "meshx_types.h"
#include "meshx_security_wrapper.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_aes_cmac(const uint8_t key[16], const uint8_t *pinput, uint32_t len,
                                    uint8_t mac[16])

MESHX_END_DECLS


#endif /* _MESHX_SECURITY_H_ */
