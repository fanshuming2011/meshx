/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <string.h>
#include "meshx_lib.h"

int32_t meshx_atoi(const char *pstr, bool *pvalid)
{
    int32_t val = 0;
    bool sign = FALSE;
    bool valid = FALSE;
    const char *p = pstr;

    /* ignore space */
    while (' ' == *p)
    {
        p ++;
        if ('\0' == *p)
        {
            if (NULL != pvalid)
            {
                *pvalid = FALSE;
            }
            return 0;
        }
    }

    /* check sign */
    if ('-' == *p)
    {
        sign = TRUE;
        p ++;
    }

    /* get val */
    while ('\0' != *p)
    {
        if ((*p >= '0') && (*p <= '9'))
        {
            valid = TRUE;
            val = val * 10 + *p - '0';
        }
        else
        {
            break;
        }
        p ++;
    }

    if (NULL != pvalid)
    {
        *pvalid = valid;
    }

    return sign ? -val : val;
}

void meshx_itoa(int32_t val, char *pstr)
{
    char *p = pstr;
    if (val < 0)
    {
        *p = '-';
        p ++;
        val = -val;
    }

    do
    {
        *p = val % 10 + '0';
        p++;
        val /= 10;
    }
    while (val > 0);

    *p = '\0';
    uint8_t count = ((val < 0) ? (p - pstr - 1) : (p - pstr)) / 2;
    p = (val < 0) ? (pstr + 1) : pstr;
    uint8_t temp;
    for (uint8_t i = 0; i < count; ++i)
    {
        temp = p[i];
        p[i] = p[count * 2  - 1 - i];
        p[count * 2 - 1 - i] = temp;
    }
}

void meshx_bin2hex(const char *pbin, uint8_t *phex, uint8_t len)
{
    memset(phex, 0, len / 2);
    for (uint8_t i = 0; i < len; ++i)
    {
        if ((pbin[i] >= 'a') && (pbin[i] <= 'f'))
        {
            phex[i / 2] |= (pbin[i] - 'a' + 10);
        }
        else if ((pbin[i] >= 'A') && (pbin[i] <= 'F'))
        {
            phex[i / 2] |= (pbin[i] - 'A' + 10);
        }
        else if ((pbin[i] >= '0') && (pbin[i] <= '9'))
        {
            phex[i / 2] |= (pbin[i] - '0');
        }
        else
        {
            break;
        }
        if (0 == (i & 0x01))
        {
            phex[i / 2] <<= 4;
        }
    }
}