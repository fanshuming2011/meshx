/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include "meshx_errno.h"

typedef struct
{
    int32_t err_code;
    const char *err_str;
} meshx_err_table_t;

static const char *meshx_err_str_table[] =
{
    "operation success",
    "operation failed",
    "out of memory",
    "invalid argument",
    "invalid adtype",
    "invalid state",
    "invalid network interface",
    "device or resource busy",
    "try again",
    "operation already done",
    "invalid parameter length",
    "reach maximum limitation",
    "bearer has not connected to network interface",
    "do not pass network interface filter",
    "value is different",
    "stop execution",
    "no resource",
    "error timing",
    "timeout",
    "no key found",
    "not found",
};

const char *meshx_errno_to_string(int32_t err)
{
    err = (err < 0) ? -err : err;
    return (err < MESHX_ERR_USER_DEFINED) ? meshx_err_str_table[err] : "unknown error code";
}
