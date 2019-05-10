/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#include <string.h>
#define TRACE_MODULE "MESHX_GAP_WRAPPER"
#include "meshx_trace.h"
#include "meshx_gap_wrapper.h"
#include "meshx_errno.h"

static uint8_t adv_buffer[64];

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
    memcpy(adv_buffer, pdata, len);
    return MESHX_SUCCESS;
}

int32_t meshx_gap_adv_set_scan_rsp_data(const uint8_t *pdata, uint8_t len)
{
    MESHX_INFO("set adv scan rsp data");
    return MESHX_SUCCESS;
}

int32_t meshx_gap_adv_start(void)
{
    MESHX_INFO("adv start");
    return MESHX_SUCCESS;
}

int32_t meshx_gap_adv_stop(void)
{
    MESHX_INFO("adv stop");
    return MESHX_SUCCESS;
}
