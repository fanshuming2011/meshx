/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_CMD_PROV_H_
#define _MESHX_CMD_PROV_H_

#include "meshx_cmd.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_cmd_beacon_scan(const meshx_cmd_parsed_data_t *pparsed_data);


#define MESHX_CMD_PROV \
    {\
        "bs",\
        "bs [beacon type]",\
        "scan device beacons, 0: stop scan 1: scan unprovisioned device beacon 2: scan secure network beacon",\
        meshx_cmd_beacon_scan \
    }


MESHX_END_DECLS

#endif /* _MESHX_CMD_CONMMON_H_ */
