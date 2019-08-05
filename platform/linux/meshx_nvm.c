/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_nvm.h"
#include "meshx_errno.h"

int32_t meshx_nvm_init(void)
{
    return MESHX_SUCCESS;
}

int32_t meshx_nvm_write(uint32_t address, const uint8_t *pdata, uint32_t len)
{
    return MESHX_SUCCESS;
}

int32_t meshx_nvm_read(uint32_t address, uint8_t *pdata, uint32_t len)
{
    return MESHX_SUCCESS;
}

