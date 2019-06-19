/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#define TRACE_MODULE "MESHX_TTY"
#include "meshx_trace.h"
#include "meshx_tty.h"
#include "meshx_errno.h"

static struct termios default_settings;

int32_t meshx_tty_init(void)
{
    struct termios new_settings;
    tcgetattr(STDIN_FILENO, &default_settings);

#if 0
    /* Disable canonical mode, and set buffer size to 1 byte */
    new_settings.c_lflag &= ~(ECHO | ICANON);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
#endif

    new_settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                              | INLCR | IGNCR | ICRNL | IXON);
    new_settings.c_oflag &= ~OPOST;
    new_settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    new_settings.c_cflag &= ~(CSIZE | PARENB);
    new_settings.c_cflag |= CS8;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_settings);

    return MESHX_SUCCESS;
}

void meshx_tty_deinit(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &default_settings);
}

int32_t meshx_tty_send(const char *pdata, uint32_t len)
{
    while (len --)
    {
        putc(*pdata, stdout);
        pdata ++;
    }
    fflush(stdout);

    return len;
}

