/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_TRACE_H_
#define _MESHX_TRACE_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

#ifndef TRACE_MODULE
#define TRACE_MODULE "APP"
#endif

#define MESHX_TRACE_LEVEL_DEBUG           0x0100
#define MESHX_TRACE_LEVEL_INFO            0x0201
#define MESHX_TRACE_LEVEL_WARN            0x0402
#define MESHX_TRACE_LEVEL_ERROR           0x0803
#define MESHX_TRACE_LEVEL_FATAL           0x1004

#define MESHX_TRACE_LEVEL_ALL             (MESHX_TRACE_LEVEL_DEBUG | MESHX_TRACE_LEVEL_INFO |\
                                           MESHX_TRACE_LEVEL_WARN | MESHX_TRACE_LEVEL_ERROR |\
                                           MESHX_TRACE_LEVEL_FATAL)

MESHX_EXTERN int32_t meshx_trace_init(void);
MESHX_EXTERN int32_t meshx_trace_send(const char *pdata, uint32_t len);
MESHX_EXTERN void meshx_trace(const char *module, uint16_t level, const char *func, const char *fmt,
                              ...);
MESHX_EXTERN void meshx_trace_dump(const char *module, uint16_t level, const char *func,
                                   const void *pdata, uint32_t len);
MESHX_EXTERN void meshx_trace_level_enable(uint16_t levels);
MESHX_EXTERN void meshx_trace_level_disable(uint16_t levels);


#define MESHX_DEBUG(fmt, ...) \
    meshx_trace(TRACE_MODULE, MESHX_TRACE_LEVEL_DEBUG, __FUNC__, fmt, ##__VA_ARGS__)
#define MESHX_INFO(fmt, ...) \
    meshx_trace(TRACE_MODULE, MESHX_TRACE_LEVEL_INFO, __FUNC__, fmt, ##__VA_ARGS__)
#define MESHX_WARN(fmt, ...) \
    meshx_trace(TRACE_MODULE, MESHX_TRACE_LEVEL_WARN, __FUNC__, fmt, ##__VA_ARGS__)
#define MESHX_ERROR(fmt, ...) \
    meshx_trace(TRACE_MODULE, MESHX_TRACE_LEVEL_ERROR, __FUNC__, fmt, ##__VA_ARGS__)
#define MESHX_FATAL(fmt, ...) \
    meshx_trace(TRACE_MODULE, MESHX_TRACE_LEVEL_FATAL, __FUNC__, fmt, ##__VA_ARGS__)


#define MESHX_DUMP_DEBUG(pdata, len) \
    meshx_trace_dump(TRACE_MODULE, MESHX_TRACE_LEVEL_DEBUG, __FUNC__, pdata, len)
#define MESHX_DUMP_INFO(pdata, len) \
    meshx_trace_dump(TRACE_MODULE, MESHX_TRACE_LEVEL_INFO, __FUNC__, pdata, len)
#define MESHX_DUMP_WARN(pdata, len) \
    meshx_trace_dump(TRACE_MODULE, MESHX_TRACE_LEVEL_WARN, __FUNC__, pdata, len)
#define MESHX_DUMP_ERROR(pdata, len) \
    meshx_trace_dump(TRACE_MODULE, MESHX_TRACE_LEVEL_ERROR, __FUNC__, pdata, len)
#define MESHX_DUMP_FATAL(pdata, len) \
    meshx_trace_dump(TRACE_MODULE, MESHX_TRACE_LEVEL_FATAL, __FUNC__, pdata, len)

MESHX_END_DECLS

#endif /* _MESHX_TRACE_H_ */
