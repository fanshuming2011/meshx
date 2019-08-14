/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NMC_H_
#define _MESHX_NMC_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef struct
{
    uint16_t src;
    uint32_t seq;
} meshx_nmc_t;

MESHX_EXTERN int32_t meshx_nmc_init(uint32_t size);
MESHX_EXTERN void meshx_nmc_deinit(void);
MESHX_EXTERN int32_t meshx_nmc_add(meshx_nmc_t nmc);
MESHX_EXTERN void meshx_nmc_clear(void);
MESHX_EXTERN bool meshx_nmc_exists(meshx_nmc_t nmc);

MESHX_END_DECLS



#endif /* _MESHX_NMC_H_ */
