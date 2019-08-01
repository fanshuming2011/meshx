/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_NVM_H_
#define _MESHX_NVM_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_nvm_init(void);
MESHX_EXTERN int32_t meshx_nvm_write(uint32_t address, const uint8_t *pdata, uint32_t len);
MESHX_EXTERN int32_t meshx_nvm_read(uint32_t address, uint8_t *pdata, uint32_t len);

MESHX_END_DECLS

#endif /* _MESHX_NVM_H_ */
