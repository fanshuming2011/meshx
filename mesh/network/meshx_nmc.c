/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_NMC"
#include "meshx_nmc.h"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_node_internal.h"

static meshx_nmc_t *nmc_array;
static uint32_t nmc_index;
static bool nmc_circle;
static uint32_t nmc_size;

int32_t meshx_nmc_init(void)
{
    if (NULL != nmc_array)
    {
        MESHX_ERROR("nmc already initialized");
        return -MESHX_ERR_ALREADY;
    }

    nmc_array = meshx_malloc(meshx_node_params.config.nmc_size * sizeof(meshx_nmc_t));
    memset(nmc_array, 0, sizeof(meshx_nmc_t) * meshx_node_params.config.nmc_size);
    if (NULL == nmc_array)
    {
        MESHX_ERROR("initialize nmc failed: out of memory");
        return -MESHX_ERR_MEM;
    }

    nmc_index = 0;
    nmc_circle = false;
    nmc_size = meshx_node_params.config.nmc_size;
    MESHX_INFO("initialize nmc module success: size %d", meshx_node_params.config.nmc_size);
    return MESHX_SUCCESS;
}

void meshx_nmc_deinit(void)
{
    if (NULL != nmc_array)
    {
        meshx_free(nmc_array);
        nmc_array = NULL;
    }

    nmc_index = 0;
    nmc_circle = false;
    nmc_size = 0;
    MESHX_INFO("deinitialize nmc module");
}

int32_t meshx_nmc_add(meshx_nmc_t nmc)
{
    if (NULL == nmc_array)
    {
        MESHX_ERROR("initialize nmc module first!");
        return -MESHX_ERR_INVAL;
    }

    if (nmc_index == nmc_size)
    {
        nmc_index = 0;
        nmc_circle = TRUE;
    }

    nmc_array[nmc_index] = nmc;
    MESHX_DEBUG("add nmc: src 0x%04x, seq 0x%06x", nmc.src, nmc.seq);

    nmc_index ++;

    return MESHX_SUCCESS;
}

void meshx_nmc_clear(void)
{
    if (NULL != nmc_array)
    {
        memset(nmc_array, 0, sizeof(meshx_nmc_t) * nmc_size);
    }
    nmc_index = 0;
    nmc_circle = FALSE;
    MESHX_INFO("clear nmc");
}

bool meshx_nmc_check(meshx_nmc_t nmc)
{
    if (NULL == nmc_array)
    {
        MESHX_ERROR("initialize nmc module first!");
        return FALSE;
    }

    uint32_t check_length = nmc_index;
    if (nmc_circle)
    {
        check_length = nmc_size;
    }

    bool ret = TRUE;
    for (uint32_t i = 0; i < check_length; ++i)
    {
        if (0 == memcmp(&nmc_array[i], &nmc, sizeof(meshx_nmc_t)))
        {
            MESHX_WARN("nmc check failed: %d, src 0x%04x, seq 0x%06x-0x%06x", nmc.src, nmc.seq,
                       nmc_array[i].seq);
            ret = FALSE;
        }
        break;
    }

    MESHX_DEBUG("nmc check passed");

    return ret;
}