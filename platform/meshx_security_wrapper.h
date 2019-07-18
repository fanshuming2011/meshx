/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_SECURITY_WRAPPER_H_
#define _MESHX_SECURITY_WRAPPER_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS


MESHX_EXTERN int32_t meshx_ecc_make_key(uint8_t public_key[64], uint8_t private_key[64]);
MESHX_EXTERN bool meshx_ecc_is_valid_public_key(uint8_t public_key[64]);
MESHX_EXTERN int32_t meshx_ecc_shared_secret(uint8_t public_key[64], uint8_t private_key[64],
                                             uint8_t secret[32]);
MESHX_EXTERN int32_t meshx_aes128_encrypt(const uint8_t input[16], const uint8_t key[16],
                                          uint8_t output[16]);
MESHX_EXTERN int32_t meshx_aes128_decrypt(const uint8_t input[16], const uint8_t key[16],
                                          uint8_t output[16]);
MESHX_EXTERN int32_t meshx_aes_cmac(const uint8_t key[16], const uint8_t *pinput, uint32_t len,
                                    uint8_t mac[16]);
MESHX_EXTERN int32_t meshx_aes_ccm_encrypt(const uint8_t key[16], const uint8_t *piv,
                                           uint32_t iv_len,
                                           const uint8_t *padd, uint32_t add_len,
                                           const uint8_t *pinput, uint32_t length, uint8_t *poutput,
                                           uint8_t *ptag, uint32_t tag_len);
MESHX_EXTERN int32_t meshx_aes_ccm_decrypt(const uint8_t key[16], const uint8_t *piv,
                                           uint32_t iv_len,
                                           const uint8_t *padd, uint32_t add_len,
                                           const uint8_t *pinput, uint32_t length, uint8_t *poutput,
                                           const uint8_t *ptag, uint32_t tag_len);



MESHX_END_DECLS

#endif /* _MESHX_SECURITY_WRAPPER_H_ */
