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

#define MESHX_NONCE_TYPE_NETWORK          0
#define MESHX_NONCE_TYPE_APPLICAITON      1
#define MESHX_NONCE_TYPE_DEVICE           2
#define MESHX_NONCE_TYPE_PROXY            3

typedef struct
{
    uint8_t nonce_type;
    uint8_t ctl: 1;
    uint8_t ttl: 7;
    uint8_t seq[3];
    uint16_t src;
    uint16_t pad;
    uint8_t iv_index[4];
} __PACKED meshx_network_nonce_t;

typedef struct
{
    uint8_t nonce_type;
    uint8_t aszmic: 1;
    uint8_t pad: 7;
    uint8_t seq[3];
    uint16_t src;
    uint16_t dst;
    uint8_t iv_index[4];
} __PACKED meshx_application_nonce_t;

typedef struct
{
    uint8_t nonce_type;
    uint8_t aszmic: 1;
    uint8_t pad: 7;
    uint8_t seq[3];
    uint16_t src;
    uint16_t dst;
    uint8_t iv_index[4];
} __PACKED meshx_device_nonce_t;

typedef struct
{
    uint8_t nonce_type;
    uint8_t pad;
    uint8_t seq[3];
    uint16_t src;
    uint16_t pad1;
    uint8_t iv_index[4];
} __PACKED meshx_proxy_nonce_t;


MESHX_EXTERN int32_t meshx_s1(const uint8_t *pM, uint32_t Mlen, uint8_t salt[16]);
MESHX_EXTERN int32_t meshx_k1(const uint8_t *pN, uint32_t Nlen, uint8_t salt[16], const uint8_t *pP,
                              uint32_t Plen, uint8_t key[16]);
MESHX_EXTERN int32_t meshx_k2(const uint8_t N[16], const uint8_t *pP, uint32_t Plen, uint8_t *pnid,
                              uint8_t encryption_key[16], uint8_t privacy_key[16]);
MESHX_EXTERN int32_t meshx_k3(const uint8_t N[16], uint8_t value[8]);
MESHX_EXTERN int32_t meshx_k4(const uint8_t N[16], uint8_t value[1]);


MESHX_END_DECLS


#endif /* _MESHX_SECURITY_H_ */
