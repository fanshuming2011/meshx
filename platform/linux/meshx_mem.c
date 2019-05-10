/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <stdlib.h>
#include "meshx_mem.h"


void *meshx_malloc(size_t size)
{
    return malloc(size);
}

void meshx_free(void *ptr)
{
    free(ptr);
}
