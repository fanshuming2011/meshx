/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "MESHX_GAP"
#include "meshx_trace.h"
#include "meshx_config.h"
#include "meshx_gap.h"
#include "meshx_bearer.h"
#include "meshx_errno.h"
#include "meshx_list.h"

typedef struct
{
    meshx_gap_action_t gap_action;
    meshx_list_t node;
} meshx_gap_action_list_t;

static meshx_gap_action_list_t gap_actions[MESHX_GAP_ACTION_MAX_NUM];

static meshx_list_t gap_action_list_idle;
static meshx_list_t gap_action_list_active;

typedef struct
{
    bool enabled;
    uint8_t adv_state;
    uint8_t scan_state;
} meshx_gap_state_t;

static meshx_gap_state_t gap_state;

static meshx_gap_adv_param_t gap_adv_param =
{
    .adv_interval_min = 0x20,
    .adv_interval_min = 0x20,
    .adv_type = MESHX_GAP_ADV_TYPE_NONCONN_IND,
    .channel = MESHX_GAP_CHANNEL_ALL
};

int32_t meshx_gap_init(void)
{
    meshx_list_init_head(&gap_action_list_idle);
    meshx_list_init_head(&gap_action_list_active);
    for (uint8_t i = 0; i < MESHX_GAP_ACTION_MAX_NUM; ++i)
    {
        meshx_list_append(&gap_action_list_idle, &gap_actions[i].node);
    }

    gap_state.enabled = FALSE;

    return MESHX_SUCCESS;
}

static void meshx_clear_all_actions(void)
{
    meshx_list_init_head(&gap_action_list_idle);
    meshx_list_init_head(&gap_action_list_active);
    for (uint8_t i = 0; i < MESHX_GAP_ACTION_MAX_NUM; ++i)
    {
        meshx_list_append(&gap_action_list_idle, &gap_actions[i].node);
    }
}

int32_t meshx_gap_start(void)
{
    if (gap_state.enabled)
    {
        MESHX_WARN("gap already started");
        return -MESHX_ERR_ALREADY;
    }
    /* start scaning */
    meshx_gap_scan_param_t param;
    param.scan_type = MESHX_GAP_SCAN_TYPE_PASSIVE;
    param.scan_window = MESHX_GAP_SCAN_WINDOW;
    param.scan_interval = MESHX_GAP_SCAN_INTERVAL;
    meshx_gap_scan_set_param(&param);
    meshx_gap_scan_start();

    gap_state.enabled = TRUE;

    return MESHX_SUCCESS;
}

void meshx_gap_stop(void)
{
    if (gap_state.enabled)
    {
        meshx_gap_scan_stop();
        meshx_gap_adv_stop();
        meshx_clear_all_actions();

        gap_state.enabled = FALSE;
    }
}

static void meshx_run_actions(void)
{
    meshx_list_t *pnode = meshx_list_peek(&gap_action_list_active);
    if (NULL != pnode)
    {
        bool action_done = FALSE;
        meshx_gap_action_list_t *paction = MESHX_CONTAINER_OF(pnode, meshx_gap_action_list_t, node);
        switch (paction->gap_action.action_type)
        {
        case MESHX_GAP_ACTION_TYPE_SCAN:
            if (MESHX_GAP_SCAN_STATE_IDLE == gap_state.scan_state)
            {
                /* set scan parameter, then start scaning */
                meshx_gap_scan_param_t param;
                param.scan_type = paction->gap_action.action_scan_data.scan_type;
                param.scan_window = paction->gap_action.action_scan_data.scan_window;
                param.scan_interval = paction->gap_action.action_scan_data.scan_interval;
                meshx_gap_scan_set_param(&param);
                meshx_gap_scan_start();
                action_done = TRUE;
            }
            break;
        case MESHX_GAP_ACTION_TYPE_ADV:
            if (MESHX_GAP_ADV_STATE_IDLE == gap_state.adv_state)
            {
                /* set adv parameter and data, then start avdertising */
                meshx_gap_adv_param_t param = gap_adv_param;
                param.adv_type = paction->gap_action.action_adv_data.adv_type;
                meshx_gap_adv_set_param(&param);
                meshx_gap_adv_set_data(paction->gap_action.action_adv_data.data,
                                       paction->gap_action.action_adv_data.data_len);
                meshx_gap_adv_start();
                paction->gap_action.action_adv_data.send_times --;
                if (0 == paction->gap_action.action_adv_data.send_times)
                {
                    action_done = TRUE;
                }
            }
            break;
        default:
            MESHX_WARN("invalid action type: %d", paction->gap_action.action_type);
            break;
        }

        if (action_done)
        {
            /* remove node and add to idle list */
            meshx_list_remove(pnode);
            meshx_list_append(&gap_action_list_idle, pnode);
        }
    }
}

int32_t meshx_gap_handle_bt_status(const meshx_gap_bt_status_t *pstatus)
{
    switch (pstatus->status_type)
    {
    case MESHX_BT_STATUS_STACK_STATE_CHANGE:
        if (pstatus->stack_state == MESHX_GAP_STACK_STATE_READY)
        {
            meshx_gap_start();
        }
        break;
    case MESHX_BT_STATUS_ADV_STATE_CHANGE:
        gap_state.adv_state = pstatus->adv_state;
        if (MESHX_GAP_ADV_STATE_IDLE == gap_state.adv_state)
        {
            meshx_run_actions();
        }
        break;
    case MESHX_BT_STATUS_SCAN_STATE_CHANGE:
        gap_state.scan_state = pstatus->scan_state;
        break;
    case MESHX_BT_STATUS_CONN_STATE_CHANGE:
        break;
    default:
        break;
    }
    return MESHX_SUCCESS;
}

static bool meshx_action_cmp(const meshx_list_t *pnode_insert, const meshx_list_t *pnode_list)
{
    meshx_gap_action_list_t *paction_list = MESHX_CONTAINER_OF(pnode_list, meshx_gap_action_list_t,
                                                               node);
    meshx_gap_action_list_t *paction_insert = MESHX_CONTAINER_OF(pnode_insert, meshx_gap_action_list_t,
                                                                 node);

    return (paction_insert->gap_action.action_type > paction_list->gap_action.action_type);
}

static int32_t meshx_sort_insert_action(const meshx_gap_action_t *paction)
{
    meshx_list_t *pnode = meshx_list_pop(&gap_action_list_idle);
    if (NULL == pnode)
    {
        MESHX_ERROR("action number reached maximum!");
        return -MESHX_ERR_BUSY;
    }

    meshx_gap_action_list_t *pact = MESHX_CONTAINER_OF(pnode, meshx_gap_action_list_t, node);
    pact->gap_action = *paction;
    meshx_list_sort_insert(&gap_action_list_active, pnode, meshx_action_cmp);

    MESHX_INFO("add adv action");
    return MESHX_SUCCESS;
}

static int32_t meshx_append_action(const meshx_gap_action_t *paction)
{
    meshx_list_t *pnode = meshx_list_pop(&gap_action_list_idle);
    if (NULL == pnode)
    {
        MESHX_ERROR("action number reached maximum!");
        return -MESHX_ERR_BUSY;
    }

    meshx_gap_action_list_t *pact = MESHX_CONTAINER_OF(pnode, meshx_gap_action_list_t, node);
    pact->gap_action = *paction;
    meshx_list_append(&gap_action_list_active, pnode);

    return MESHX_SUCCESS;
}

int32_t meshx_gap_add_action(const meshx_gap_action_t *paction)
{
    if (!gap_state.enabled)
    {
        MESHX_WARN("add action failed: gap not started!");
        return -MESHX_ERR_STATE;
    }

    int32_t ret = MESHX_SUCCESS;

    switch (paction->action_type)
    {
    case MESHX_GAP_ACTION_TYPE_SCAN:
        ret = meshx_append_action(paction);
        break;
    case MESHX_GAP_ACTION_TYPE_ADV:
        ret = meshx_sort_insert_action(paction);
        break;
    default:
        ret = -MESHX_ERR_INVAL;
        MESHX_WARN("invalid action type: %d", paction->action_type);
        break;
    }

    if (MESHX_SUCCESS == ret)
    {
        meshx_run_actions();
    }

    return ret;
}

int32_t meshx_gap_handle_adv_report(const uint8_t *pdata, uint16_t len,
                                    const meshx_bearer_rx_metadata_t *prx_metadata)
{
    if (len > MESHX_GAP_ADV_DATA_MAX_LEN || len < 2)
    {
        MESHX_DEBUG("invalid length: %d", len);
        return -MESHX_ERR_LENGTH;
    }

    uint8_t data_len = MESHX_GAP_GET_ADV_LEN(pdata);
    if (data_len != len - MESHX_GAP_ADV_LENGTH_FIELD_SIZE)
    {
        MESHX_DEBUG("received length does not match calculated length: %d-%d",
                    len - MESHX_GAP_ADV_LENGTH_FIELD_SIZE, data_len);
        return -MESHX_ERR_LENGTH;
    }

    return meshx_bearer_receive(pdata, len, prx_metadata);
}

