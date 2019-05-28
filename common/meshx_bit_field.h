/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_BIT_FIELD_H_
#define _MESHX_BIT_FIELD_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

#define MESHX_BIT_FIELD_SET(field, bit)         field[bit / 8] |= (1 << (bit %8))
#define MESHX_BIT_FIELD_CLEAR(field, bit)       field[bit / 8] &= ~(1 << (bit % 8))
#define MESHX_IS_BIT_FIELD_SET(field, bit)      ((field[bit / 8] & (1 << (bit % 8))) != 0)


MESHX_END_DECLS



#endif /* _MESHX_BIT_FIELD_H_ */
