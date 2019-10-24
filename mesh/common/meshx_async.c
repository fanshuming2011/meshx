/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "MESHX_ASYNC"
#include "meshx_trace.h"
#include "meshx_async.h"
#include "meshx_async_internal.h"
#include "meshx_provision_internal.h"
#include "meshx_errno.h"
#include "meshx_list.h"
#include "meshx_mem.h"
#include "meshx_beacon_internal.h"
#include "meshx_trans_internal.h"

static meshx_async_msg_notify_t meshx_async_msg_notify;

typedef struct
{
    meshx_async_msg_t msg;
    meshx_list_t node;
} meshx_async_msg_list_t;

static meshx_list_t async_msg_active_list;
static meshx_list_t async_msg_idle_list;

static int32_t meshx_default_msg_notify(void)
{
    meshx_async_msg_process();

    return MESHX_SUCCESS;
}

int32_t meshx_async_msg_init(uint32_t msg_max_num, meshx_async_msg_notify_t handler)
{
    if (NULL == handler)
    {
        MESHX_INFO("use default async message notify");
        meshx_async_msg_notify = meshx_default_msg_notify;
    }
    else
    {
        MESHX_INFO("use user async message notify");
        meshx_async_msg_notify = handler;
    }
    meshx_list_init_head(&async_msg_active_list);
    meshx_list_init_head(&async_msg_idle_list);

    meshx_async_msg_list_t *pmsg = meshx_malloc(sizeof(meshx_async_msg_list_t) * msg_max_num);
    if (NULL == pmsg)
    {
        MESHX_ERROR("initialize async message failed: out of memory");
        return -MESHX_ERR_MEM;
    }

    for (uint32_t i = 0; i < msg_max_num; ++i)
    {
        meshx_list_append(&async_msg_idle_list, &pmsg[i].node);
    }

    return MESHX_SUCCESS;
}

static meshx_async_msg_list_t *meshx_async_msg_request(void)
{
    meshx_async_msg_list_t *pmsg = NULL;
    meshx_list_t *pnode = meshx_list_pop(&async_msg_idle_list);
    if (NULL != pnode)
    {
        pmsg = MESHX_CONTAINER_OF(pnode, meshx_async_msg_list_t, node);
    }

    return pmsg;
}

static void meshx_async_msg_release(meshx_async_msg_list_t *pmsg)
{
    meshx_list_append(&async_msg_idle_list, &pmsg->node);
}

int32_t meshx_async_msg_send(const meshx_async_msg_t *pmsg)
{
    meshx_async_msg_list_t *pmsg_node = meshx_async_msg_request();
    if (NULL == pmsg_node)
    {
        MESHX_ERROR("send async message failed: out of resource");
        return -MESHX_ERR_RESOURCE;
    }
    pmsg_node->msg = *pmsg;
    meshx_list_append(&async_msg_active_list, &pmsg_node->node);
    meshx_async_msg_notify();
    return MESHX_SUCCESS;
}

void meshx_async_msg_process(void)
{
    meshx_list_t *pnode = meshx_list_pop(&async_msg_active_list);
    if (NULL == pnode)
    {
        MESHX_INFO("no message need to process");
        return ;
    }
    meshx_async_msg_list_t *pmsg = MESHX_CONTAINER_OF(pnode, meshx_async_msg_list_t, node);
    MESHX_INFO("processing message: %d", pmsg->msg.type);
    switch (pmsg->msg.type)
    {
    case MESHX_ASYNC_MSG_TYPE_TIMEOUT_PB_ADV:
        meshx_pb_adv_async_handle_timeout(pmsg->msg);
        break;
    case MESHX_ASYNC_MSG_TYPE_TIMEOUT_BEACON:
        meshx_beacon_async_handle_timeout(pmsg->msg);
        break;
    case MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_TX:
        meshx_lower_trans_async_handle_tx_timeout(pmsg->msg);
        break;
    case MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_RX_ACK:
        meshx_lower_trans_async_handle_rx_ack_timeout(pmsg->msg);
        break;
    case MESHX_ASYNC_MSG_TYPE_TIMEOUT_LOWER_TRANS_RX_INCOMPLETE:
        meshx_lower_trans_async_handle_rx_incomplete_timeout(pmsg->msg);
        break;
    default:
        MESHX_ERROR("unkonwn message type: %d", pmsg->msg.type);
        break;
    }

    meshx_async_msg_release(pmsg);
}
