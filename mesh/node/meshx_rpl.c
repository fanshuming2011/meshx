/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MESHX_RPL"
#include "meshx_rpl.h"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_mem.h"

static meshx_rpl_t *rpl_array;
static uint32_t rpl_index;
static uint32_t rpl_size;

int32_t meshx_rpl_init(uint32_t size)
{
    if (NULL != rpl_array)
    {
        MESHX_ERROR("rpl already initialized");
        return -MESHX_ERR_ALREADY;
    }

    rpl_array = meshx_malloc(size * sizeof(meshx_rpl_t));
    memset(rpl_array, 0, sizeof(meshx_rpl_t) * size);
    if (NULL == rpl_array)
    {
        MESHX_ERROR("initialize rpl failed: out of memory");
        return -MESHX_ERR_MEM;
    }

    rpl_index = 0;
    rpl_size = size;
    return MESHX_SUCCESS;
}

void meshx_rpl_deinit(void)
{
    if (NULL != rpl_array)
    {
        meshx_free(rpl_array);
        rpl_array = NULL;
    }

    rpl_index = 0;
    rpl_size = 0;
}

int32_t meshx_rpl_update(meshx_rpl_t rpl)
{
    if (NULL == rpl_array)
    {
        MESHX_ERROR("initialize rpl module first!");
        return -MESHX_ERR_INVAL;
    }

    for (uint8_t i = 0; i < rpl_index; ++i)
    {
        if (rpl_array[rpl_index].src == rpl.src)
        {
            /* found exists rpl */
            rpl_array[rpl_index].seq = rpl.seq;
            return MESHX_SUCCESS;
        }
    }

    /* not found, try to add */
    if (rpl_index == rpl_size)
    {
        MESHX_ERROR("rpl list is full, can't add new rpl!");
        return -MESHX_ERR_RESOURCE;
    }

    rpl_array[rpl_index] = rpl;

    rpl_index ++;

    return MESHX_SUCCESS;
}

void meshx_rpl_clear(void)
{
    if (NULL != rpl_array)
    {
        memset(rpl_array, 0, sizeof(meshx_rpl_t) * rpl_size);
    }
    rpl_index = 0;
}

bool meshx_rpl_exists(meshx_rpl_t rpl)
{
    if (NULL == rpl_array)
    {
        MESHX_WARN("need initialize rpl module");
        return FALSE;
    }

    for (uint32_t i = 0; i < rpl_index; ++i)
    {
        if ((rpl_array[i].src == rpl.src) && (rpl.seq <= rpl_array[i].seq))
        {
            return TRUE;
        }
    }

    return FALSE;
}