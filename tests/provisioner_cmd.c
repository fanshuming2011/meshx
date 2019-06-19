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


int32_t meshx_cmd_prov_scan(const meshx_cmd_parsed_data_t *pparsed_data)
{
    return MESHX_SUCCESS;
}

int32_t meshx_cmd_prov_conn(const meshx_cmd_parsed_data_t *pparsed_data)
{
    return MESHX_SUCCESS;
}

int32_t meshx_cmd_prov_invite(const meshx_cmd_parsed_data_t *pparsed_data)
{
    return MESHX_SUCCESS;
}

static meshx_cmd_t prov_cmds[] =
{
    MESHX_CMD_BASE,
    MESHX_CMD_COMMON,
    {
        "prov_scan",
        "prov_scan [state]",
        "scan unprovisioned beacon, state: 0-hide udb 1-show udb",
        meshx_cmd_prov_scan,
    },
    {
        "prov_conn",
        "prov_conn [uuid]",
        "connect unprovisioned device",
        meshx_cmd_prov_conn,
    },
    {
        "prov_invite",
        "prov_invite []",
        "provision invite",
        meshx_cmd_prov_invite,
    }
};

void provisioner_cmd_init(void)
{
    meshx_cmd_init(prov_cmds, sizeof(prov_cmds) / sizeof(meshx_cmd_t));
}


