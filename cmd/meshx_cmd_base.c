/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx_cmd_base.h"
#include "meshx_errno.h"

int32_t meshx_cmd_help(const meshx_cmd_parsed_data_t *pparsed_data)
{
    if (0 == pparsed_data->param_cnt)
    {
        return -MESHX_ERR_INVAL;
    }

    if (0 == strcmp(pparsed_data->param_ptr[0], "*"))
    {
        meshx_cmd_list_all();
    }
    else
    {
        meshx_cmd_list(pparsed_data->param_ptr[0]);
    }

    return MESHX_SUCCESS;
}

int32_t meshx_cmd_reboot(const meshx_cmd_parsed_data_t *pparsed_data)
{
    return MESHX_SUCCESS;
}

