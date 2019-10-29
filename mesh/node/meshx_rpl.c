/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_RPL"
#include "meshx_rpl.h"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_node_internal.h"

static meshx_rpl_t *rpl_array;
static uint32_t rpl_index;
static uint32_t rpl_size;

int32_t meshx_rpl_init(void)
{
    if (NULL != rpl_array)
    {
        MESHX_ERROR("rpl already initialized");
        return -MESHX_ERR_ALREADY;
    }

    rpl_array = meshx_malloc(meshx_node_params.config.rpl_size * sizeof(meshx_rpl_t));
    memset(rpl_array, 0, sizeof(meshx_rpl_t) * meshx_node_params.config.rpl_size);
    if (NULL == rpl_array)
    {
        MESHX_ERROR("initialize rpl failed: out of memory");
        return -MESHX_ERR_MEM;
    }

    rpl_index = 0;
    rpl_size = meshx_node_params.config.rpl_size;
    MESHX_INFO("initialize rpl module success: size %d", meshx_node_params.config.rpl_size);

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
    MESHX_INFO("deinitialize rpl module");
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
            rpl_array[rpl_index].iv_index = rpl.iv_index;
            MESHX_DEBUG("update rpl: src 0x%04x, seq 0x%06x, iv index 0x%08x", rpl_array[rpl_index].src,
                        rpl_array[rpl_index].seq, rpl_array[rpl_index].iv_index);
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
    MESHX_DEBUG("update rpl: src 0x%04x, seq 0x%06x, iv index 0x%08x", rpl_array[rpl_index].src,
                rpl_array[rpl_index].seq, rpl_array[rpl_index].iv_index);

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
    MESHX_INFO("clear rpl list");
}

bool meshx_rpl_check(meshx_rpl_t rpl)
{
    if (NULL == rpl_array)
    {
        MESHX_ERROR("initialize rpl module first!");
        return FALSE;
    }

    bool ret = TRUE;
    for (uint32_t i = 0; i < rpl_index; ++i)
    {
        if (rpl_array[i].src == rpl.src)
        {
            if ((rpl.seq <= rpl_array[i].seq) || (rpl.iv_index < rpl_array[i].iv_index))
            {
                MESHX_WARN("rpl check failed: src 0x%04x, seq 0x%06x-0x%06x, iv index 0x%08x-0x%08x", rpl.src,
                           rpl_array[i].seq, rpl.seq, rpl_array[i].iv_index, rpl.iv_index);
                ret = FALSE;
            }
            break;
        }
    }

    MESHX_DEBUG("rpl check passed");
    return ret;
}

bool meshx_rpl_is_full(void)
{
    if (NULL != rpl_array)
    {
        MESHX_ERROR("initialize rpl module first!");
        return TRUE;
    }

    return (rpl_index >= rpl_size);
}
