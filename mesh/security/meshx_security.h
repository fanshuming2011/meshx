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

MESHX_EXTERN int32_t meshx_s1(const uint8_t *pM, uint32_t Mlen, uint8_t salt[16]);
MESHX_EXTERN int32_t meshx_k1(const uint8_t *pN, uint32_t Nlen, uint8_t salt[16], const uint8_t *pP,
                              uint32_t Plen, uint8_t key[16]);
MESHX_EXTERN int32_t meshx_k2(const uint8_t N[16], const uint8_t *pP, uint32_t Plen, uint8_t T1[16],
                              uint8_t T2[16], uint8_t T3[16], uint8_t id[0]);
MESHX_EXTERN int32_t meshx_k3(const uint8_t N[16], uint8_t value[8]);
MESHX_EXTERN int32_t meshx_k4(const uint8_t N[16], uint8_t value[1]);


MESHX_END_DECLS


#endif /* _MESHX_SECURITY_H_ */
