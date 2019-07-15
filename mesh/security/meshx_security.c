/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_security.h"
#include "meshx_errno.h"

int32_t meshx_sw_aes_cmac(const uint8_t key[16], const uint8_t *pinput, uint32_t len,
                          uint8_t mac[16])
{
    int mbedtls_cipher_cmac(const mbedtls_cipher_info_t *cipher_info,
                            const unsigned char *key, size_t keylen,
                            const unsigned char *input, size_t ilen,
                            unsigned char *output)
    return MESHX_SUCCESS;
}

int32_t meshx_sw_aes_ccm(void)
{
    return MESHX_SUCCESS;
}
