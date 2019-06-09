/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_TTY_H_
#define _MESHX_TTY_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_tty_init(void);
MESHX_EXTERN void meshx_tty_deinit(void);
MESHX_EXTERN uint32_t meshx_tty_send(const char *pdata, uint32_t len);

MESHX_END_DECLS


#endif /* _MESHX_TTY_H_ */
