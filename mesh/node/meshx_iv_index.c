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
#include "meshx_async.h"
#include "meshx_async_internal.h"
#include "meshx_trans_internal.h"
#include "meshx_seq.h"
#include "meshx_rpl.h"

static uint32_t meshx_iv_index;
static meshx_iv_update_state_t meshx_iv_update_state;
static bool meshx_iv_test_mode_enabled;
static bool meshx_iv_update_state_transit_pending;


/* iv operate time parameters */
#define MESHX_IV_OPERATE_TICK_PERIOD           1200 /* unit is second */
#define MESHX_IV_OPERATE_TICK_96H                   (96 * 3600 / MESHX_IV_OPERATE_TICK_PERIOD)
#define MESHX_IV_OPERATE_TICK_144H                  (144 * 3600 / MESHX_IV_OPERATE_TICK_PERIOD)
#define MESHX_IV_OPERATE_TICK_192H                  (2 * MESHX_IV_OPERATE_TICK_96H)
#define MESHX_IV_OPERATE_TICK_48W                   (42 * MESHX_IV_OPERATE_TICK_192H)
static meshx_timer_t meshx_iv_operate_tick_timer;
static uint32_t meshx_iv_operate_tick_time;

#define MESHX_IV_INDEX_MAX_DIFF                42


static void meshx_iv_operate_tick_timeout(void *pargs)
{
    meshx_async_msg_t msg;
    msg.type = MESHX_ASYNC_MSG_TYPE_TIMEOUT_IV_INDEX;
    msg.pdata = pargs;
    msg.data_len = 0;
    meshx_async_msg_send(&msg);
}

int32_t meshx_iv_index_init(void)
{
    MESHX_INFO("initialize iv index module...");
    meshx_timer_create(&meshx_iv_operate_tick_timer, MESHX_TIMER_MODE_REPEATED,
                       meshx_iv_operate_tick_timeout, NULL);
    if (NULL == meshx_iv_operate_tick_timer)
    {
        MESHX_ERROR("create iv index operate timer failed!");
        return -MESHX_ERR_RESOURCE;
    }
    meshx_timer_start(meshx_iv_operate_tick_timer, MESHX_IV_OPERATE_TICK_PERIOD * 1000);

    return MESHX_SUCCESS;
}

void meshx_iv_index_deinit(void)
{
    MESHX_INFO("deinitialize iv index module...");
    if (NULL != meshx_iv_operate_tick_timer)
    {
        meshx_timer_delete(meshx_iv_operate_tick_timer);
        meshx_iv_operate_tick_timer = NULL;
    }

    meshx_iv_index = 0;
    meshx_iv_update_state = MESHX_IV_UPDATE_STATE_NORMAL;
    meshx_iv_test_mode_enabled = FALSE;
    meshx_iv_update_state_transit_pending = FALSE;
    meshx_iv_operate_tick_time = 0;
}

void meshx_iv_index_async_handle_timeout(meshx_async_msg_t msg)
{
    if (meshx_iv_operate_tick_time < MESHX_IV_OPERATE_TICK_48W)
    {
        meshx_iv_operate_tick_time ++;
    }

    if ((meshx_iv_operate_tick_time >= MESHX_IV_OPERATE_TICK_144H) &&
        (MESHX_IV_UPDATE_STATE_IN_PROGRESS == meshx_iv_update_state))
    {
        /* check whether send segment message */
        if (meshx_is_lower_trans_busy())
        {
            meshx_iv_update_state_transit_pending = TRUE;
            MESHX_INFO("iv update state transit to normal is pending: iv index 0x%08x", meshx_iv_index);
        }
        else
        {
            /* return to normal state */
            meshx_iv_update_state = MESHX_IV_UPDATE_STATE_NORMAL;
            meshx_iv_operate_tick_time = 0;
            meshx_seq_clear();
            meshx_rpl_clear();
            MESHX_INFO("iv update state transit to normal: iv index 0x%08x", meshx_iv_index);
        }
    }
}

uint32_t meshx_iv_index_get(void)
{
    return meshx_iv_index;
}

uint32_t meshx_iv_index_tx_get(void)
{
    return (MESHX_IV_UPDATE_STATE_IN_PROGRESS == meshx_iv_update_state) ? (meshx_iv_index - 1) :
           meshx_iv_index;
}

void meshx_iv_index_set(uint32_t iv_index)
{
    meshx_iv_index = iv_index;
}

int32_t meshx_iv_index_update(uint32_t iv_index, meshx_iv_update_state_t state)
{
    if (meshx_iv_index > iv_index)
    {
        MESHX_ERROR("received iv index(0x%08x) is less than current(0x%08x)", iv_index, meshx_iv_index);
        return -MESHX_ERR_INVAL;
    }

    int32_t distance = iv_index - meshx_iv_index;
    if (distance > MESHX_IV_INDEX_MAX_DIFF)
    {
        MESHX_WARN("node has been left the network over 48 weeks, need reprovisioned!");
        return -MESHX_ERR_MAX;
    }

    distance = (distance << 1) + !state - !meshx_iv_update_state;
    if (distance <= 0)
    {
        return MESHX_SUCCESS;
    }

    if (!meshx_iv_test_mode_enabled &&
        (meshx_iv_operate_tick_time < (distance * MESHX_IV_OPERATE_TICK_96H)))
    {
        MESHX_ERROR("operate time(%d) is not enough to change state: %d", meshx_iv_operate_tick_time);
        return -MESHX_ERR_TIMING;
    }

    meshx_iv_index = iv_index;
    if (1 == distance)
    {
        /* normal iv update procedure */
        if (MESHX_IV_UPDATE_STATE_NORMAL == state)
        {
            /* check whether send segment message */
            if (meshx_is_lower_trans_busy())
            {
                meshx_iv_update_state_transit_pending = TRUE;
                MESHX_INFO("iv update state transit to normal is pending: iv index 0x%08x", meshx_iv_index);
            }
            else
            {
                /* return to normal state */
                meshx_iv_update_state = state;
                meshx_iv_operate_tick_time = 0;
                meshx_seq_clear();
                meshx_rpl_clear();
                MESHX_INFO("iv update state transit to normal: iv index 0x%08x", meshx_iv_index);
            }
        }
        else
        {
            meshx_iv_update_state = state;
            MESHX_INFO("iv update state transit to in progress: iv index 0x%08x", meshx_iv_index);
        }
    }
    else
    {
        /* iv recovery procudure */
        meshx_iv_update_state = state;
        meshx_iv_operate_tick_time = MESHX_IV_OPERATE_TICK_96H;
        meshx_seq_clear();
        meshx_rpl_clear();
        MESHX_INFO("iv index recovery: iv index 0x%08x, state %d", meshx_iv_index, state);
    }

    return MESHX_SUCCESS;
}

void meshx_iv_update_state_set(meshx_iv_update_state_t state)
{
    meshx_iv_update_state = state;
}

meshx_iv_update_state_t meshx_iv_update_state_get(void)
{
    return meshx_iv_update_state;
}

void meshx_iv_update_operate_time_set(uint32_t time)
{
    meshx_iv_operate_tick_time = time / MESHX_IV_OPERATE_TICK_PERIOD;
}

void meshx_iv_test_mode_enable(bool flag)
{
    meshx_iv_test_mode_enabled = flag;
}

bool meshx_is_iv_update_state_transit_pending(void)
{
    return meshx_iv_update_state_transit_pending;
}