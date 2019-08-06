/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_SEQ"
#include "meshx_seq.h"
#include "meshx_trace.h"

static uint32_t meshx_seq = 0;

uint32_t meshx_seq_get(void)
{
    return meshx_seq;
}

void meshx_seq_set(uint32_t seq)
{
    meshx_seq = seq;
}

void meshx_seq_increase(void)
{
    meshx_seq ++;
}
