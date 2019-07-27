
/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx_io.h"

#define CHAR_MAX_LEN              3
#define HEX_MAX_LEN               8
#define DEC_MAX_LEN               10

static int32_t output_numner(io_write pwrite, uint8_t len, uint8_t width, uint8_t max_len,
                             bool zeropad, bool spacepad,
                             const char *data)
{
    int32_t count = 0;
    int32_t step_count = 0;
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
                step_count = pwrite("0", 1);
                if (step_count > 0)
                {
                    count += step_count;
                }
            }
            else if (spacepad)
            {
                step_count = pwrite(" ", 1);
                if (step_count > 0)
                {
                    count += step_count;
                }
            }
        }
        else
        {
            step_count = pwrite(&data[i - 1], 1);
            if (step_count > 0)
            {
                count += step_count;
            }
        }
    }

    return count;
}

int meshx_vsprintf(io_write pwrite, const char *fmt, va_list args)
{
    if (NULL == pwrite)
    {
        return -1;
    }

    int width = 0;
    bool zeropad = FALSE;
    bool spacepad = FALSE;
    int32_t write_count = 0;
    int32_t step_count = 0;

    for (; *fmt != '\0'; ++fmt)
    {
        if (*fmt != '%')
        {
            step_count = pwrite(fmt, 1);
            if (step_count > 0)
            {
                write_count += step_count;
            }
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

                step_count = pwrite(pdata, len);
                if (step_count > 0)
                {
                    write_count += step_count;
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
                if (0 == num)
                {
                    hex[count] = '0';
                    count ++;
                }
                else
                {
                    while (num)
                    {
                        hex[count] = ("0123456789ABCDEF"[num & 0x0f] | lower_case);
                        count ++;
                        num >>= 4;
                    }
                }
                step_count = output_numner(pwrite, count, width, HEX_MAX_LEN, zeropad, spacepad, hex);
                if (step_count > 0)
                {
                    write_count += step_count;
                }
            }
            break;
        case 'c':
            {
                char num = va_arg(args, int);
                while (-- width > 0)
                {
                    step_count = pwrite(" ", 1);
                    if (step_count > 0)
                    {
                        write_count += step_count;
                    }
                }
                step_count = pwrite(&num, 1);
                if (step_count > 0)
                {
                    write_count += step_count;
                }
            }
            break;
        case 'd':
            {
                int num = va_arg(args, int);
                if (num < 0)
                {
                    step_count = pwrite("-", 1);
                    num = ~num + 1;
                    if (step_count > 0)
                    {
                        write_count += step_count;
                    }
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
                step_count = output_numner(pwrite, count, width, DEC_MAX_LEN, zeropad, spacepad, dec);
                if (step_count > 0)
                {
                    write_count += step_count;
                }
            }
            break;
        default:
            step_count = pwrite(fmt, 1);
            if (step_count > 0)
            {
                write_count += step_count;
            }
            break;
        }
    }

    return write_count;
}

int meshx_printf(io_write pwrite, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int32_t count = meshx_vsprintf(pwrite, fmt, args);
    va_end(args);

    return count;
}
