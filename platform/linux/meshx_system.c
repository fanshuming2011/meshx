/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include "meshx_system.h"

/**
 * @brief exit function of tlog
 * @param code - exit code
 */
void meshx_exit(int code)
{
    /* use exit() instead _exit() if you want to flush I/O buffer)*/
    _exit(code);
}

/**
 * @brief print backtrace
 */
void meshx_print_backtrace(void)
{
    void *bt[64];
    int bt_size = 0;
    int i = 0;
    char **syms = NULL;

    bt_size = backtrace(bt, 500);
    syms = backtrace_symbols(bt, bt_size);

    while (i < bt_size)
    {
        fprintf(stderr, "%s\n", syms[i]);
        ++i;
    }
    fflush(stderr);

    free(syms);
}


/**
 * @brief abort function of tlog
 */
void meshx_abort(void)
{
    meshx_print_backtrace();

    abort();
    /* in case someone manages to ignore SIGABRT ? */
    meshx_exit(1);
}

