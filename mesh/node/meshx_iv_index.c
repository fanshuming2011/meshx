/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_IV_INDEX"
#include "meshx_iv_index.h"

static uint32_t meshx_iv_index;

uint32_t meshx_iv_index_get(void)
{
    return meshx_iv_index;
}

uint32_t meshx_iv_index_set(uint32_t iv_index)
{
    if (meshx_iv_index < iv_index)
    {
        meshx_iv_index = iv_index;
    }

    return meshx_iv_index;
}
