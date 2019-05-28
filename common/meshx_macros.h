/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_MACROS_H_
#define _MESHX_MACROS_H_

#include "meshx_compiler.h"


/* guard C code in headers, while including them from C++ */
#ifdef  __cplusplus
#define MESHX_BEGIN_DECLS  extern "C" {
#define MESHX_END_DECLS    }
#else
#define MESHX_BEGIN_DECLS
#define MESHX_END_DECLS
#endif

#define MESHX_MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define MESHX_MIN(a, b)  (((a) < (b)) ? (a) : (b))

#define MESHX_ABS(a)     (((a) < 0) ? -(a) : (a))

#define MESHX_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/*
 * Count the number of elements in an array.
 * The array can not be a dynamically allocated array
 */
#define MESHX_N_ELEMENTS(arr)       (sizeof(arr) / sizeof((arr)[0]))

/* handling structure */
#define MESHX_OFFSET_OF(struct_type, member)    \
    ((unsigned long)&((struct_type *)0)->member)

/* get struct address */
#define MESHX_CONTAINER_OF(member_ptr, struct_type, member)                     \
    (struct_type *)((char *)member_ptr - MESHX_OFFSET_OF(struct_type, member))

/* function name */
#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define __FUNC__ __func__
#elif defined(__GNUC__)
#define __FUNC__ __PRETTY_FUNCTION__
#else
#define __FUNC__ "unknown function"
#endif


#define MESHX_UNUSED(data)   (void)(data)

#define _STR(s)   #s
#define MESHX_STR(s)    _STR(s)

/* export definition */
#ifndef MESHX_EXTERN
#define MESHX_EXTERN extern
#endif

#endif /* _MESHX_MACROS_H_ */
