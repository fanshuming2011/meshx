/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_BEACON_H_
#define _MESHX_BEACON_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS

#define MESHX_BEACON_TYPE_UDB      0
#define MESHX_BEACON_TYPE_SNB      1

#define MESHX_UDB_OOB_OTHER                     (1 << 0)
#define MESHX_UDB_OOB_URI                       (1 << 1)
#define MESHX_UDB_OOB_2D_CODE                   (1 << 2)
#define MESHX_UDB_OOB_BAR_CODE                  (1 << 3)
#define MESHX_UDB_OOB_NFC                       (1 << 4)
#define MESHX_UDB_OOB_NUMBER                    (1 << 5)
#define MESHX_UDB_OOB_STRING                    (1 << 6)
#define MESHX_UDB_OOB_ON_BOX                    (1 << 11)
#define MESHX_UDB_OOB_INSIDE_BOX                (1 << 12)
#define MESHX_UDB_OOB_ON_PIECE_OF_PAPER         (1 << 13)
#define MESHX_UDB_OOB_INSIDE_MANUAL             (1 << 14)
#define MESHX_UDB_OOB_ON_DEVICE                 (1 << 15)

typedef struct
{
    uint8_t type;
    meshx_dev_uuid_t dev_uuid;
    uint16_t oob_info;
    uint32_t uri_hash; /* optional */
} __PACKED meshx_udb_t;


MESHX_EXTERN int32_t meshx_beacon_send(meshx_bearer_t bearer, uint8_t beacon_type);
MESHX_EXTERN int32_t meshx_beacon_start(meshx_bearer_t bearer, uint8_t beacon_type,
                                        uint32_t interval);
MESHX_EXTERN void meshx_beacon_stop(uint8_t beacon_type);
MESHX_EXTERN void meshx_beacon_set_oob_info(uint16_t oob_info);
MESHX_EXTERN void meshx_beacon_set_uri_hash(uint32_t uri_hash);



MESHX_END_DECLS


#endif /* _MESHX_BEACON_H */
