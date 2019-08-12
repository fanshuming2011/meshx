/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MESHX_SECURITY"
#include "meshx_trace.h"
#include "meshx_mem.h"
#include "meshx_security.h"
#include "meshx_errno.h"

int32_t meshx_s1(const uint8_t *pdata, uint32_t len, uint8_t salt[16])
{
    if (NULL == pdata)
    {
        return -MESHX_ERR_INVAL;
    }

    uint8_t key[16];
    memset(key, 0, 16);
    meshx_aes_cmac(key, pdata, len, salt);

    return MESHX_SUCCESS;
}

int32_t meshx_k1(const uint8_t *pN, uint32_t Nlen, uint8_t salt[16], const uint8_t *pP,
                 uint32_t Plen, uint8_t key[16])
{
    if ((NULL == pN) || (NULL == pP))
    {
        return -MESHX_ERR_INVAL;
    }

    uint8_t key_T[16];
    meshx_aes_cmac(salt, pN, Nlen,  key_T);
    meshx_aes_cmac(key_T, pP, Plen, key);

    return MESHX_SUCCESS;
}

int32_t meshx_k2(const uint8_t N[16], const uint8_t *pP, uint32_t Plen, uint8_t *pnid,
                 uint8_t encryption_key[16], uint8_t privacy_key[16])
{
    if (NULL == pP)
    {
        return -MESHX_ERR_INVAL;
    }

    uint8_t *pt_tmp = meshx_malloc(Plen + 17);
    if (NULL == pt_tmp)
    {
        return -MESHX_ERR_MEM;
    }
    memcpy(pt_tmp, pP, Plen);

    uint8_t salt[16];
    uint8_t smk2[] = {'s', 'm', 'k', '2'};
    meshx_s1(smk2, sizeof(smk2), salt);
    uint8_t key_T[16];
    meshx_aes_cmac(salt, N, 16,  key_T);
    MESHX_DEBUG("key_T:");
    MESHX_DUMP_DEBUG(key_T, 16);

    uint8_t T1[16], T2[16], T3[16];

    /* generate T1  */
    pt_tmp[Plen] = 0x01;
    meshx_aes_cmac(key_T, pt_tmp, Plen + 1, T1);

    /* generate T2  */
    memcpy(pt_tmp, T1, 16);
    memcpy(pt_tmp + 16, pP, Plen);
    pt_tmp[Plen + 16] = 0x02;
    meshx_aes_cmac(key_T, pt_tmp, Plen + 17, T2);

    /* generate T3  */
    memcpy(pt_tmp, T2, 16);
    memcpy(pt_tmp + 16, pP, Plen);
    pt_tmp[Plen + 16] = 0x03;
    meshx_aes_cmac(key_T, pt_tmp, Plen + 17, T3);

    /* generate id */
    *pnid = (T1[15] & 0x7f);
    memcpy(encryption_key, T2, sizeof(T2));
    memcpy(privacy_key, T3, sizeof(T3));

    return MESHX_SUCCESS;
}

int32_t meshx_k3(const uint8_t N[16], uint8_t value[8])
{
    uint8_t salt[16];
    uint8_t smk3[] = {'s', 'm', 'k', '3'};
    meshx_s1(smk3, sizeof(smk3), salt);
    uint8_t key_T[16];
    meshx_aes_cmac(salt, N, 16,  key_T);
    uint8_t id64[] = {'i', 'd', '6', '4', 0x01};
    uint8_t out[16];
    meshx_aes_cmac(key_T, id64, sizeof(id64), out);
    memcpy(value, out + 8, 8);

    return MESHX_SUCCESS;
}

int32_t meshx_k4(const uint8_t N[16], uint8_t value[1])
{
    uint8_t salt[16];
    uint8_t smk4[] = {'s', 'm', 'k', '4'};
    meshx_s1(smk4, sizeof(smk4), salt);
    uint8_t key_T[16];
    meshx_aes_cmac(salt, N, 16,  key_T);
    uint8_t id64[] = {'i', 'd', '6', 0x01};
    uint8_t out[16];
    meshx_aes_cmac(key_T, id64, sizeof(id64), out);
    value[0] = out[15] & 0x3f;

    return MESHX_SUCCESS;
}

int32_t meshx_e(const uint8_t input[16], const uint8_t key[16], uint8_t output[16])
{
    return meshx_aes128_encrypt(input, key, output);
}