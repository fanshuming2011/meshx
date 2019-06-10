/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "DEVICE"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include "meshx.h"
#include "msg_queue.h"
#include "meshx_async_internal.h"
//#include "meshx_mem.h"

meshx_msg_queue_t msg_queue = NULL;

#define ASYNC_DATA_TYPE_ADV_DATA   0
#define ASYNC_DATA_TYPE_MESH_DATA  1
#define ASYNC_DATA_TYPE_TTY        2

typedef struct
{
    uint8_t type;
    uint8_t data[32];
    uint8_t data_len;
} async_data_t;


#define FIFO_DSPR  "/tmp/fifo_dspr"
#define FIFO_PSDR  "/tmp/fifo_psdr"


static pthread_t meshx_tid;
static pthread_t meshx_receive;
static pthread_t console_receive;
int fd_dspr;
int fd_psdr;

typedef void (*trace_send)(const char *pdata, uint32_t len);

static FILE *log_file;

void system_init(void)
{
    mkfifo(FIFO_DSPR, 0777);
    mkfifo(FIFO_PSDR, 0777);
    fd_psdr = open(FIFO_PSDR, O_RDONLY);
    fd_dspr = open(FIFO_DSPR, O_WRONLY);
    log_file = fopen("./log_dev", "w");
    meshx_tty_init();
    meshx_cmd_init(NULL, 0);
}

void linux_send_string(const char *pdata, uint32_t len)
{
    fwrite(pdata, 1, len, log_file);
    fflush(log_file);
}

static int32_t meshx_prov_cb(const meshx_provision_dev_t prov_dev, uint8_t type, void *pargs)
{
    switch (type)
    {
    case MESHX_PROVISION_CB_TYPE_LINK_OPEN:
        {
            meshx_provision_link_open_t *pdata = pargs;
            MESHX_DEBUG("link opened, result: %d", *pdata);
        }
        break;
    case MESHX_PROVISION_CB_TYPE_LINK_CLOSE:
        {
            meshx_provision_link_close_t *pdata = pargs;
            MESHX_DEBUG("link closed, result: %d", *pdata);
        }
        break;
    case MESHX_PROVISION_CB_TYPE_INVITE:
        {
            meshx_provision_invite_t *pdata = pargs;
            MESHX_DEBUG("invite: %d", pdata->attention_duration);
            /* send capabilites */
            meshx_provision_capabilites_t cap;
            memset(&cap, 0, sizeof(meshx_provision_capabilites_t));
            meshx_provision_capabilites(prov_dev, &cap);
        }
        break;
    case MESHX_PROVISION_CB_TYPE_START:
        {
            meshx_provision_start_t *pdata = pargs;
            MESHX_DEBUG("start:");
            MESHX_DUMP_DEBUG(pdata, sizeof(meshx_provision_start_t));
            /* send capabilites */
            /*
            meshx_provision_capabilites_t cap;
            memset(&cap, 0, sizeof(meshx_provision_capabilites_t));
            meshx_provision_capabilites(prov_dev, &cap);
            */
        }
        break;
    case MESHX_PROVISION_CB_TYPE_FAILED:
        {
            /* @ref meshx provisison failed error code macros */
            MESHX_DEBUG("provision failed");
        }
        break;
    case MESHX_PROVISION_CB_TYPE_COMPLETE:
        {
            MESHX_DEBUG("provision complete");
        }
        break;
    }
    return MESHX_SUCCESS;
}

static int32_t meshx_async_msg_notify_handler(const meshx_async_msg_t *pmsg)
{
    async_data_t async_data;
    async_data.type = ASYNC_DATA_TYPE_MESH_DATA;
    *((uint64_t *)async_data.data) = (uint64_t)pmsg;
    async_data.data_len = 8;
    msg_queue_send(msg_queue, &async_data, sizeof(async_data_t));

    return MESHX_SUCCESS;
}

static void *meshx_thread(void *pargs)
{
    msg_queue_create(&msg_queue, 10, sizeof(async_data_t));
    meshx_async_msg_set_notify(meshx_async_msg_notify_handler);


    meshx_trace_init(linux_send_string);
    meshx_trace_level_enable(MESHX_TRACE_LEVEL_ALL);
    meshx_gap_init();
    meshx_bearer_init();
    meshx_network_init();
    meshx_provision_init(meshx_prov_cb);

    meshx_gap_start();
    meshx_bearer_param_t adv_param = {.bearer_type = MESHX_BEARER_TYPE_ADV, .param_adv.adv_period = 0};
    meshx_bearer_t adv_bearer = meshx_bearer_create(adv_param);

    meshx_network_if_t adv_network_if = meshx_network_if_create();
    meshx_network_if_connect(adv_network_if, adv_bearer, NULL, NULL);

    meshx_dev_uuid_t uuid;
    for (uint8_t i = 0; i < 16; ++i)
    {
        uuid[i] = i;
    }
    meshx_set_device_uuid(uuid);

    meshx_bearer_rx_metadata_t rx_metadata;
    meshx_bearer_rx_metadata_adv_t adv_metadata =
    {
        .adv_type = MESHX_GAP_ADV_TYPE_NONCONN_IND,
        .peer_addr = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66},
        .addr_type = MESHX_GAP_PUBLIC_ADDR,
        .channel = MESHX_GAP_CHANNEL_39,
        .rssi = 10,
    };
    rx_metadata.bearer_type = MESHX_BEARER_TYPE_ADV;
    rx_metadata.adv_metadata = adv_metadata;

    while (1)
    {
        async_data_t async_data;
        if (MESHX_SUCCESS == msg_queue_receive(msg_queue, &async_data, -1))
        {
            switch (async_data.type)
            {
            case ASYNC_DATA_TYPE_ADV_DATA:
                /* data read finished */
                MESHX_DEBUG("receive adv data:");
                MESHX_DUMP_DEBUG(async_data.data, async_data.data_len);
                meshx_gap_handle_adv_report(async_data.data, async_data.data_len, &rx_metadata);
                break;
            case ASYNC_DATA_TYPE_MESH_DATA:
                {
                    uint64_t address = *((uint64_t *)async_data.data);
                    meshx_async_msg_t *pmsg = (meshx_async_msg_t *)address;
                    MESHX_DEBUG("receive mesh inner msg: %d", pmsg->type);
                    meshx_async_msg_process(pmsg);
                }
                break;
            case ASYNC_DATA_TYPE_TTY:
                {
                    meshx_cmd_parse(async_data.data, 1);
                }
                break;
            default:
                break;
            }
        }
    }
    pthread_exit((void *)0);
}

static void *meshx_receive_thread(void *pargs)
{
    int data_len = 0;
    uint8_t adv_recv_data[64];

    while (1)
    {
        data_len = read(fd_psdr, adv_recv_data, 31);
        async_data_t async_data;
        async_data.type = ASYNC_DATA_TYPE_ADV_DATA;
        memcpy(async_data.data, adv_recv_data, data_len);
        async_data.data_len = data_len;
        msg_queue_send(msg_queue, &async_data, sizeof(async_data_t));

    }
    pthread_exit((void *)0);
}

static void *console_receive_thread(void *pargs)
{
    int data = 0;

    while (1)
    {
        data = getc(stdin);
        if (data != EOF)
        {
            async_data_t async_data;
            async_data.type = ASYNC_DATA_TYPE_TTY;
            async_data.data[0] = data;
            async_data.data_len = 1;
            msg_queue_send(msg_queue, &async_data, sizeof(async_data_t));
        }
    }
    pthread_exit((void *)0);
}

int main(int argc, char **argv)
{
    system_init();
    printf("device system initialized success!\r\n");

    pthread_create(&meshx_tid, NULL, meshx_thread, NULL);
    pthread_create(&meshx_receive, NULL, meshx_receive_thread, NULL);
    pthread_create(&console_receive, NULL, console_receive_thread, NULL);
    pthread_join(meshx_tid, NULL);
    pthread_join(meshx_receive, NULL);
    pthread_join(console_receive, NULL);

    return 0;
}

