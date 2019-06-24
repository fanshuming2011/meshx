/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "meshx_errno.h"
#include "meshx_trace.h"
#include "meshx_config.h"
#include "meshx_io.h"
#include "meshx_trace_io.h"

static uint8_t trace_active_levels;

/* trace level names */
#define TRACE_LEVEL_NAME_LEN      5
static char *trace_level_names[] =
{
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL",
};

int32_t meshx_trace_init(void)
{
    return meshx_trace_io_init();
}

void meshx_trace_level_enable(uint16_t levels)
{
    levels >>= 8;
    trace_active_levels |= levels;
}

void meshx_trace_level_disable(uint16_t levels)
{
    levels >>= 8;
    trace_active_levels &= ~levels;
}

#if MESHX_TRACE_FAST
void meshx_trace(const char *module, uint16_t level, const char *func, const char *fmt, ...)
{
}
#else
void meshx_trace(const char *module, uint16_t level, const char *func, const char *fmt, ...)
{
    /* check level */
    if (trace_active_levels & (level >> 8))
    {
        /* output time */
        time_t lt = time(NULL);
        struct tm ltm;
        localtime_r(&lt, &ltm);
        char time_cur[32];
        size_t time_len = strftime(time_cur, 32, "%F %T", &ltm);
        meshx_trace_send(time_cur, time_len);
        meshx_trace_send(".", 1);
        struct timeval tv;
        gettimeofday(&tv, NULL);
        char temp_buf[6];
        temp_buf[0] = tv.tv_usec / 100000 + '0';
        temp_buf[1] = tv.tv_usec / 10000 % 10 + '0';
        temp_buf[2] = tv.tv_usec / 1000 % 10 + '0';
        temp_buf[3] = tv.tv_usec / 100 % 10 + '0';
        temp_buf[4] = tv.tv_usec / 10 % 10 + '0';
        temp_buf[5] = tv.tv_usec % 10 + '0';
        meshx_trace_send(temp_buf, 6);
        meshx_trace_send(" ", 1);


        /* output level */
        meshx_trace_send(trace_level_names[level & 0x0f], TRACE_LEVEL_NAME_LEN);
        /* output seperator */
        meshx_trace_send(" [", 2);
        /*output module and function */
        meshx_trace_send(module, strlen(module));
        meshx_trace_send(" ", 1);
        meshx_trace_send(func, strlen(func));
        /* output seperator */
        meshx_trace_send("] ", 2);
        /* output message */
        va_list args;
        va_start(args, fmt);
        meshx_vsprintf(meshx_trace_send, fmt, args);
        va_end(args);
        /* output CR */
        meshx_trace_send("\r\n", 2);
    }
}
#endif

void meshx_trace_dump(const char *module, uint16_t level, const char *func, const void *pdata,
                      uint32_t len)
{
    /* check level */
    if (trace_active_levels & (level >> 8))
    {
        /* output time */
        time_t lt = time(NULL);
        struct tm ltm;
        localtime_r(&lt, &ltm);
        char time_cur[32];
        size_t time_len = strftime(time_cur, 32, "%F %T", &ltm);
        meshx_trace_send(time_cur, time_len);
        meshx_trace_send(".", 1);
        struct timeval tv;
        gettimeofday(&tv, NULL);
        char temp_buf[6];
        temp_buf[0] = tv.tv_usec / 100000 + '0';
        temp_buf[1] = tv.tv_usec / 10000 % 10 + '0';
        temp_buf[2] = tv.tv_usec / 1000 % 10 + '0';
        temp_buf[3] = tv.tv_usec / 100 % 10 + '0';
        temp_buf[4] = tv.tv_usec / 10 % 10 + '0';
        temp_buf[5] = tv.tv_usec % 10 + '0';
        meshx_trace_send(temp_buf, 6);
        meshx_trace_send(" ", 1);

        /* output level */
        meshx_trace_send(trace_level_names[level & 0x0f], TRACE_LEVEL_NAME_LEN);
        /* output seperator */
        meshx_trace_send(" [", 2);
        /*output module and function */
        meshx_trace_send(module, strlen(module));
        meshx_trace_send(" ", 1);
        meshx_trace_send(func, strlen(func));
        /* output seperator */
        meshx_trace_send("] ", 2);

        for (uint32_t index = 0; index < len; ++ index)
        {
            char data = "0123456789ABCDEF"[((const uint8_t *)pdata)[index] >> 4];
            meshx_trace_send(&data, 1);
            data = "0123456789ABCDEF"[((const uint8_t *)pdata)[index] & 0x0f];
            meshx_trace_send(&data, 1);
            /* output seperator */
            meshx_trace_send(" ", 1);
        }
        meshx_trace_send("\r\n", 2);
    }
}

