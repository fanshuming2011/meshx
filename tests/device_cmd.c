/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "device_cmd.h"
#include "meshx_cmd.h"
#include "meshx_cmd_base.h"
#include "meshx_cmd_common.h"
#include "meshx_cmd_prov.h"

static meshx_cmd_t dev_cmds[] =
{
    MESHX_CMD_BASE,
    MESHX_CMD_COMMON,
    MESHX_CMD_PROV,
};


void device_cmd_init(void)
{
    meshx_cmd_init(dev_cmds, sizeof(dev_cmds) / sizeof(meshx_cmd_t));
}
