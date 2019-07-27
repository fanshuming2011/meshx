/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "provisioner_cmd.h"
#include "meshx.h"
#include "meshx_cmd_base.h"
#include "meshx_cmd_common.h"
#include "meshx_cmd_prov.h"

static meshx_cmd_t prov_cmds[] =
{
    MESHX_CMD_BASE,
    MESHX_CMD_COMMON,
    MESHX_CMD_PROV,
};

void provisioner_cmd_init(void)
{
    meshx_cmd_init(prov_cmds, sizeof(prov_cmds) / sizeof(meshx_cmd_t));
}


