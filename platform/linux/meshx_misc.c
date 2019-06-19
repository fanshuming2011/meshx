/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <stdlib.h>
#include "meshx_misc.h"

void meshx_srand(uint32_t seed)
{
    srand(seed);
}

int32_t meshx_rand(void)
{
    return rand();
}

