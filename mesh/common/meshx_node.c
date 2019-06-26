/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx_node.h"

static meshx_dev_uuid_t dev_uuid;
static meshx_node_provision_state_t prov_state;

void meshx_set_device_uuid(const meshx_dev_uuid_t uuid)
{
    memcpy(dev_uuid, uuid, sizeof(meshx_dev_uuid_t));
}

void meshx_get_device_uuid(meshx_dev_uuid_t uuid)
{
    memcpy(uuid, dev_uuid, sizeof(meshx_dev_uuid_t));
}

uint16_t meshx_get_node_address(void)
{
    return MESHX_ADDRESS_UNASSIGNED;
}

void meshx_set_node_prov_state(meshx_node_provision_state_t state)
{
    prov_state = state;
}

meshx_node_provision_state_t meshx_get_node_prov_state(void)
{
    return prov_state;
}
