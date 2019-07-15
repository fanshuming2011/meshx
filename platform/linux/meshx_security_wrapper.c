/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx_security_wrapper.h"
#include "meshx_errno.h"
#include "aes.h"
#include "aes_cmac.h"


int32_t meshx_aes128_encrypt(const uint8_t input[16], const uint8_t key[16], uint8_t output[16])
{
    memcpy(output, input, 16);
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    AES_ECB_encrypt(&ctx, output);

    return MESHX_SUCCESS;
}

int32_t meshx_aes128_decrypt(const uint8_t input[16], const uint8_t key[16], uint8_t output[16])
{
    memcpy(output, input, 16);
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    AES_ECB_decrypt(&ctx, output);

    return MESHX_SUCCESS;
}

int32_t meshx_aes_cmac(const uint8_t key[16], const uint8_t *pinput, uint32_t len,
                       uint8_t mac[16])
{
    AES_CMAC((uint8_t *)key, (uint8_t *)pinput, len, mac);

    return MESHX_SUCCESS;
}

void AES_128(const unsigned char key[16], const unsigned char input[16], unsigned char output[16])
{
    meshx_aes128_encrypt(input, key, output);
}