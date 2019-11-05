/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define MESHX_TRACE_MODULE "MESHX_SEQ"
#include "meshx_seq.h"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_mem.h"
#include "meshx_assert.h"

static uint32_t *seq_array;
static uint8_t meshx_element_num;

int32_t meshx_seq_init(uint8_t element_num)
{
    if (NULL != seq_array)
    {
        MESHX_ERROR("sequence module has already been initialized!");
        return -MESHX_ERR_ALREADY;
    }

    seq_array = meshx_malloc(element_num * sizeof(uint32_t));
    if (NULL == seq_array)
    {
        MESHX_ERROR("initialize sequence module failed: out of memory!");
        return -MESHX_ERR_MEM;
    }
    memset(seq_array, 0, element_num * sizeof(uint32_t));
    meshx_element_num = element_num;

    return MESHX_SUCCESS;
}

void meshx_seq_deinit(void)
{
    if (NULL != seq_array)
    {
        meshx_free(seq_array);
        seq_array = NULL;
    }
    meshx_element_num = 0;
}

uint32_t meshx_seq_get(uint8_t element_index)
{
    MESHX_ASSERT(NULL != seq_array);

#if MESHX_REDUNDANCY_CHECK
    if (element_index >= meshx_element_num)
    {
        MESHX_ERROR("element index(%d) exceeded maximum element num(%d)", element_index, meshx_element_num);
        return MESHX_SEQ_INVALID;
    }
#endif

    return seq_array[element_index];
}

uint32_t meshx_seq_set(uint8_t element_index, uint32_t seq)
{
    MESHX_ASSERT(NULL != seq_array);

#if MESHX_REDUNDANCY_CHECK
    if (element_index >= meshx_element_num)
    {
        MESHX_ERROR("element index(%d) exceeded maximum element num(%d)", element_index, meshx_element_num);
        return MESHX_SEQ_INVALID;
    }
#endif

    if (seq > MESHX_SEQ_MAX)
    {
        seq = MESHX_SEQ_MAX;
    }

    seq_array[element_index] = seq;

    return seq;
}

uint32_t meshx_seq_use(uint8_t element_index)
{
    MESHX_ASSERT(NULL != seq_array);

#if MESHX_REDUNDANCY_CHECK
    if (element_index >= meshx_element_num)
    {
        MESHX_ERROR("element index(%d) exceeded maximum element num(%d)", element_index, meshx_element_num);
        return MESHX_SEQ_INVALID;
    }
#endif

    uint32_t seq = seq_array[element_index];
    if (MESHX_SEQ_MAX == seq)
    {

    }
    else
    {
        seq_array[element_index] ++;
    }

    return seq;
}

void meshx_seq_clear(void)
{
    MESHX_ASSERT(NULL != seq_array);

    for (uint8_t i = 0; i < meshx_element_num; ++i)
    {
        seq_array[i] = 0;
    }
}