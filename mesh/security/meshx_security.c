/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_security.h"
#include "meshx_errno.h"

int32_t meshx_aes_cmac(const uint8_t key[16], const uint8_t *pinput, uint32_t len, uint8_t mac[16])
{
    return MESHX_SUCCESS;
}