/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#define TRACE_MODULE "MESHX_GAP_WRAPPER"
#include "meshx_trace.h"
#include "meshx_gap_wrapper.h"
#include "meshx_errno.h"

extern int fd_psdr;
typedef struct
{
    uint8_t data[64];
    uint8_t len;
} adv_data_t;

static adv_data_t adv_data;

int32_t meshx_gap_scan_set_param(const meshx_gap_scan_param_t *pscan_param)
{
    MESHX_INFO("set scan param");
    return MESHX_SUCCESS;
}

int32_t meshx_gap_scan_start(void)
{
    MESHX_INFO("scan start");
    return MESHX_SUCCESS;
}

int32_t meshx_gap_scan_stop(void)
{
    MESHX_INFO("scan stop");
    return MESHX_SUCCESS;
}

int32_t meshx_gap_adv_set_param(const meshx_gap_adv_param_t *padv_param)
{
    MESHX_INFO("set adv param");
    return MESHX_SUCCESS;
}

int32_t meshx_gap_adv_set_data(const uint8_t *pdata, uint8_t len)
{
    MESHX_INFO("set adv data");
    memcpy(adv_data.data, pdata, len);
    adv_data.len = len;
    return MESHX_SUCCESS;
}

int32_t meshx_gap_adv_set_scan_rsp_data(const uint8_t *pdata, uint8_t len)
{
    MESHX_INFO("set adv scan rsp data");
    return MESHX_SUCCESS;
}

int32_t meshx_gap_adv_start(void)
{
    write(fd_psdr, adv_data.data, adv_data.len);
    return MESHX_SUCCESS;
}

int32_t meshx_gap_adv_stop(void)
{
    MESHX_INFO("adv stop");
    return MESHX_SUCCESS;
}
