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

#define MESHX_UDB_OOB_OTHER                     0
#define MESHX_UDB_OOB_URI                       1
#define MESHX_UDB_OOB_2D_CODE                   2
#define MESHX_UDB_OOB_BAR_CODE                  3
#define MESHX_UDB_OOB_NFC                       4
#define MESHX_UDB_OOB_NUMBER                    5
#define MESHX_UDB_OOB_STRING                    6
#define MESHX_UDB_OOB_ON_BOX                    11
#define MESHX_UDB_OOB_INSIDE_BOX                12
#define MESHX_UDB_OOB_ON_PIECE_OF_PAPER         13
#define MESHX_UDB_OOB_INSIDE_MANUAL             14
#define MESHX_UDB_OOB_ON_DEVICE                 15

MESHX_EXTERN int32_t meshx_beacon_start(meshx_bearer_t bearer, uint8_t beacon_type,
                                        uint32_t interval);
MESHX_EXTERN void meshx_beacon_stop(uint8_t beacon_type);



MESHX_END_DECLS


#endif /* _MESHX_BEACON_H */
