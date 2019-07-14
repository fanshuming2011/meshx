/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_security_wrapper.h"
#include "meshx_errno.h"
#include "mbedtls/aes.h"
#include "mbedtls/compat-1.3.h"


int32_t meshx_aes128_crypt(const uint8_t input[16], const uint8_t key[16], uint8_t output[16])
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key, 128);
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, input, output);

    return MESHX_SUCCESS;
}

int32_t meshx_aes128_decrypt(const uint8_t input[16], const uint8_t key[16], uint8_t output[16])
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_dec(&ctx, key, 128);
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, input, output);

    return MESHX_SUCCESS;
}
