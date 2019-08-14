/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MESHX_SEQ"
#include "meshx_seq.h"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_mem.h"

static uint32_t *seq_array;

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

    return MESHX_SUCCESS;
}

void meshx_seq_deinit(void)
{
    if (NULL != seq_array)
    {
        meshx_free(seq_array);
        seq_array = NULL;
    }
}

uint32_t meshx_seq_get(uint8_t element_index)
{
    if (NULL == seq_array)
    {
        MESHX_ERROR("initialize sequence module first!");
        return MESHX_SEQ_INVALID;
    }

    return seq_array[element_index];
}

uint32_t meshx_seq_set(uint8_t element_index, uint32_t seq)
{
    if (NULL == seq_array)
    {
        MESHX_ERROR("initialize sequence module first!");
        return MESHX_SEQ_INVALID;
    }

    if (seq > MESHX_SEQ_MAX)
    {
        seq = MESHX_SEQ_MAX;
    }

    seq_array[element_index] = seq;

    return seq;
}

uint32_t meshx_seq_use(uint8_t element_index)
{
    if (NULL == seq_array)
    {
        MESHX_ERROR("initialize sequence module first!");
        return MESHX_SEQ_INVALID;
    }

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
