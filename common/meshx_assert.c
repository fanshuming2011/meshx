/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <stdio.h>
#include "meshx_assert.h"
#define TRACE_MODULE "ASSERT"
#include "meshx_trace.h"
#include "meshx_system.h"

#if MESHX_ENABLE_ASSERT
/**
 * @brief assert function
 * @param[in] condition - assert condition result
 * @param[in] condition_text - assert condition text
 * @param[in] file - file of the assert
 * @param[in] line - line of the assert
 * @param[in] func - function of the assert
 */
void meshx_assert(bool condition, const char *condition_text,
                  const char *file, int line, const char *func)
{
    if (!condition)
    {
        MESHX_FATAL("assertion failed \"%s\" [file: %s] [line: %d] [function: %s]",
                    condition_text, file, line, func);
        meshx_abort();
    }
}
#endif