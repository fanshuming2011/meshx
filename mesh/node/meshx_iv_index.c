/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "MESHX_IV_INDEX"
#include "meshx_iv_index.h"
#include "meshx_timer.h"
#include "meshx_errno.h"
#include "meshx_trace.h"

static uint32_t meshx_iv_index;
static meshx_iv_update_state_t meshx_iv_update_state;
static bool meshx_iv_test_enable;


/* iv operate time parameters */
#define MESHX_IV_OPERATE_TICK_PERIOD           1200 /* unit is second */
#define MESHX_IV_OPERATE_96H                   (96 * 3600 / MESHX_IV_OPERATE_TICK_PERIOD)
#define MESHX_IV_OPERATE_144H                  (144 * 3600 / MESHX_IV_OPERATE_TICK_PERIOD)
#define MESHX_IV_OPERATE_192H                  (2 * MESHX_IV_OPERATE_96H)
#define MESHX_IV_OPERATE_48W                   (42 * MESHX_IV_OPERATE_192H)
static meshx_timer_t meshx_iv_operate_tick_timer;
static uint16_t meshx_iv_operate_time;

int32_t meshx_iv_index_init(void)
{
    meshx_timer_create(&meshx_iv_operate_tick_timer, MESHX_TIMER_MODE_REPEATED,
                       meshx_iv_operate_tick_timeout, NULL);
    if (NULL == meshx_iv_operate_tick_timer)
    {
        MESHX_ERROR("create iv index operate timer failed!");
        return -MESHX_ERR_RESOURCE;
    }
    meshx_timer_start(meshx_iv_operate_tick_timer, MESHX_IV_OPERATE_TICK_PERIOD * 1000);

    MESHX_INFO("initialize iv index module success");
    return MESHX_SUCCESS;
}

void meshx_iv_operate_tick_timeout(void *pargs)
{

}

uint32_t meshx_iv_index_get(void)
{
    return meshx_iv_index;
}

uint32_t meshx_tx_iv_index_get(void)
{
    return (MESHX_IV_UPDATE_FLAG_IN_PROGRESS == meshx_iv_update_flag) ? (meshx_iv_index - 1) :
           meshx_iv_index;
}

uint32_t meshx_iv_index_set(uint32_t iv_index)
{
    if (meshx_iv_index < iv_index)
    {
        meshx_iv_index = iv_index;
    }

    return meshx_iv_index;
}

int32_t meshx_iv_update_state_transit(meshx_iv_update_state_t flag)
{

}

meshx_iv_update_state_t meshx_iv_update_state_get(void)
{

}

meshx_iv_test_enable(bool flag)
{
    meshx_iv_test_enable = flag;
}