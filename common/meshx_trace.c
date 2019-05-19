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

static trace_send psend_func;
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

#define CHAR_MAX_LEN              3
#define HEX_MAX_LEN               8
#define DEC_MAX_LEN               10

static void output_numner(uint8_t len, uint8_t width, uint8_t max_len, bool zeropad, bool spacepad,
                          const char *data)
{
    if (width)
    {
        /* have width*/
        if (len > width)
        {
            len = width;
        }
        else
        {
            len = (width > max_len) ? max_len : width;
        }
    }
    for (uint8_t i = len; i > 0; --i)
    {
        if (data[i - 1] == 0)
        {
            if (zeropad)
            {
                psend_func("0", 1);
            }
            else if (spacepad)
            {
                psend_func(" ", 1);
            }
        }
        else
        {
            psend_func(&data[i - 1], 1);
        }
    }
}

static void meshx_trace_vsprintf(const char *fmt, va_list args)
{
    if (NULL == psend_func)
    {
        return ;
    }

    int width = 0;
    bool zeropad = FALSE;
    bool spacepad = FALSE;

    for (; *fmt != '\0'; ++fmt)
    {
        if (*fmt != '%')
        {
            psend_func(fmt, 1);
            continue;
        }

        fmt ++;

        zeropad = FALSE;
        spacepad = FALSE;
        /* get width */
        if ((*fmt  >= '0') && (*fmt  <= '9'))
        {
            if (*fmt == '0')
            {
                zeropad = TRUE;
            }
            for (; (*fmt >= '0') && (*fmt <= '9'); ++fmt)
            {
                width = width * 10 + *fmt - '0';
            }
        }

        if (width)
        {
            if (!zeropad)
            {
                spacepad = TRUE;
            }
        }

        switch (*fmt)
        {
        case 's':
            {
                zeropad = FALSE;
                char *pdata = va_arg(args, char *);
                if ('\0' == *pdata)
                {
                    pdata = "<NULL>";
                }

                int len;
                if (width > 0)
                {
                    len = strnlen(pdata, width);
                }
                else
                {
                    len = strlen(pdata);
                }

                for (int i = 0; i < len; ++i)
                {
                    psend_func(pdata + i, 1);
                }
            }
            break;
        case 'x':
        case 'X':
            {
                uint8_t lower_case = (*fmt & 0x20);
                unsigned int num = va_arg(args, unsigned int);
                char hex[HEX_MAX_LEN];
                memset(hex, 0, HEX_MAX_LEN);
                uint8_t count = 0;
                while (num & 0x0f)
                {
                    hex[count] = ("0123456789ABCDEF"[num & 0x0f] | lower_case);
                    count ++;
                    num >>= 4;
                }
                output_numner(count, width, HEX_MAX_LEN, zeropad, spacepad, hex);
            }
            break;
        case 'c':
            {
                char num = va_arg(args, int);
                while (-- width > 0)
                {
                    psend_func(" ", 1);
                }
                psend_func(&num, 1);
            }
            break;
        case 'd':
            {
                int num = va_arg(args, int);
                if (num < 0)
                {
                    psend_func("-", 1);
                    num = ~num + 1;
                }
                char dec[DEC_MAX_LEN];
                memset(dec, 0, DEC_MAX_LEN);
                uint8_t count = 0;
                do
                {
                    dec[count] = num % 10 + '0';
                    count ++;
                    num /= 10;
                }
                while (num > 0);
                output_numner(count, width, DEC_MAX_LEN, zeropad, spacepad, dec);
            }
            break;
        default:
            psend_func(fmt, 1);
            break;
        }
    }
}

int32_t meshx_trace_init(trace_send psend)
{
    psend_func = psend;
    return MESHX_SUCCESS;
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
    if (NULL != psend_func)
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
            psend_func(time_cur, time_len);
            psend_func(".", 1);
            struct timeval tv;
            gettimeofday(&tv, NULL);
            char temp_buf[6];
            temp_buf[0] = tv.tv_usec / 100000 + '0';
            temp_buf[1] = tv.tv_usec / 10000 % 10 + '0';
            temp_buf[2] = tv.tv_usec / 1000 % 10 + '0';
            temp_buf[3] = tv.tv_usec / 100 % 10 + '0';
            temp_buf[4] = tv.tv_usec / 10 % 10 + '0';
            temp_buf[5] = tv.tv_usec % 10 + '0';
            psend_func(temp_buf, 6);
            psend_func(" ", 1);


            /* output level */
            psend_func(trace_level_names[level & 0x0f], TRACE_LEVEL_NAME_LEN);
            /* output seperator */
            psend_func(" [", 2);
            /*output module and function */
            psend_func(module, strlen(module));
            psend_func(" ", 1);
            psend_func(func, strlen(func));
            /* output seperator */
            psend_func("] ", 2);
            /* output message */
            va_list args;
            va_start(args, fmt);
            meshx_trace_vsprintf(fmt, args);
            va_end(args);
            /* output CR */
            psend_func("\r\n", 2);
        }
    }
}
#endif

void meshx_trace_dump(const char *module, uint16_t level, const char *func, const uint8_t *pdata,
                      uint32_t len)
{
    if (NULL != psend_func)
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
            psend_func(time_cur, time_len);
            psend_func(".", 1);
            struct timeval tv;
            gettimeofday(&tv, NULL);
            char temp_buf[6];
            temp_buf[0] = tv.tv_usec / 100000 + '0';
            temp_buf[1] = tv.tv_usec / 10000 % 10 + '0';
            temp_buf[2] = tv.tv_usec / 1000 % 10 + '0';
            temp_buf[3] = tv.tv_usec / 100 % 10 + '0';
            temp_buf[4] = tv.tv_usec / 10 % 10 + '0';
            temp_buf[5] = tv.tv_usec % 10 + '0';
            psend_func(temp_buf, 6);
            psend_func(" ", 1);

            /* output level */
            psend_func(trace_level_names[level & 0x0f], TRACE_LEVEL_NAME_LEN);
            /* output seperator */
            psend_func(" [", 2);
            /*output module and function */
            psend_func(module, strlen(module));
            psend_func(" ", 1);
            psend_func(func, strlen(func));
            /* output seperator */
            psend_func("] ", 2);

            for (uint32_t index = 0; index < len; ++ index)
            {
                char data = "0123456789ABCDEF"[pdata[index] >> 4];
                psend_func(&data, 1);
                data = "0123456789ABCDEF"[pdata[index] & 0x0f];
                psend_func(&data, 1);
                /* output seperator */
                psend_func(" ", 1);
            }
            psend_func("\r", 1);
            psend_func("\n", 1);
        }
    }
}

