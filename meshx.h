/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_H_
#define _MESHX_H_

#include "meshx_types.h"
#include "meshx_errno.h"
#include "meshx_io.h"
#include "meshx_lib.h"
#include "meshx_trace.h"
#include "meshx_async.h"
#include "meshx_tty.h"
#include "meshx_cmd.h"
#include "meshx_gap.h"
#include "meshx_bearer.h"
#include "meshx_network.h"
#include "meshx_provision.h"
#include "meshx_node.h"
#include "meshx_beacon.h"
#include "meshx_notify.h"
#include "meshx_common.h"
#include "meshx_mem.h"


MESHX_BEGIN_DECLS

typedef struct
{
    meshx_dev_uuid_t dev_uuid;
    bool adv_bearer_enable;
    bool gatt_bearer_enable;
    uint32_t udb_interval;
    uint32_t snb_interval;
} meshx_config_t;

MESHX_EXTERN void meshx_config(uint8_t role, const meshx_config_t *pconfig);
MESHX_EXTERN meshx_config_t meshx_get_default_config(void);
MESHX_EXTERN int32_t meshx_init(void);
MESHX_EXTERN int32_t meshx_run(void);

MESHX_END_DECLS


#endif /* _MESHX_H_ */

