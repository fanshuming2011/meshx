/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "PROVISIONER"
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
#include "provisioner_cmd.h"
//#include "meshx_mem.h"


meshx_bearer_t adv_bearer;
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
    fd_psdr = open(FIFO_PSDR, O_WRONLY);
    fd_dspr = open(FIFO_DSPR, O_RDONLY);
    log_file = fopen("./log_prov", "w");
    meshx_tty_init();
    provisioner_cmd_init();
}

int32_t meshx_trace_send(const char *pdata, uint32_t len)
{
    fwrite(pdata, 1, len, log_file);
    fflush(log_file);
    return len;
}

static int32_t meshx_notify_prov_cb(const void *pdata, uint8_t len)
{
    const meshx_notify_prov_t *pprov = pdata;

    switch (pprov->metadata.prov_type)
    {
    case MESHX_PROV_NOTIFY_LINK_OPEN:
        {
            meshx_tty_printf("link opened, result: %d\r\n", pprov->link_open_result);
        }
        break;
    case MESHX_PROV_NOTIFY_LINK_CLOSE:
        {
            meshx_tty_printf("link closed, reason: %d\r\n", pprov->link_close_reason);
        }
        break;
    case MESHX_PROV_NOTIFY_CAPABILITES:
        {
            MESHX_DEBUG("capabilites:");
            MESHX_DUMP_DEBUG(&pprov->capabilites, sizeof(meshx_provision_capabilites_t));
            /* send start */
            meshx_provision_start_t start;
            memset(&start, 0, sizeof(meshx_provision_start_t));
            meshx_provision_start(pprov->metadata.prov_dev, &start);
        }
        break;
    case MESHX_PROV_NOTIFY_FAILED:
        {
            /* @ref meshx provisison failed error code macros */
            MESHX_DEBUG("provision failed");
        }
        break;
    case MESHX_PROV_NOTIFY_COMPLETE:
        {
            MESHX_DEBUG("provision complete");
        }
        break;
    }
    return MESHX_SUCCESS;

}

static int32_t meshx_notify_beacon_cb(const void *pdata, uint8_t len)
{
    const meshx_notify_beacon_t *pbeacon = pdata;
    switch (pbeacon->type)
    {
    case MESHX_NOTIFY_BEACON_TYPE_UDB:
        meshx_tty_printf("bt addr: ");
        meshx_tty_dump(pbeacon->padv_metadata->peer_addr, sizeof(meshx_mac_addr_t));
        meshx_tty_send("  ", 2);
        meshx_tty_printf("udb: ");
        meshx_tty_dump(pbeacon->udb.dev_uuid, sizeof(meshx_dev_uuid_t));
        meshx_tty_send("\r\n", 2);
        break;
    case MESHX_NOTIFY_BEACON_TYPE_PB_GATT:
        break;
    case MESHX_NOTIFY_BEACON_TYPE_PROXY:
        break;
    default:
        break;
    }
    if (len == sizeof(meshx_notify_beacon_t))
    {
    }

    return MESHX_SUCCESS;
}

static int32_t meshx_notify_cb(uint8_t notify_type, const void *pdata, uint8_t len)
{
    switch (notify_type)
    {
    case MESHX_NOTIFY_TYPE_PROV:
        meshx_notify_prov_cb(pdata, len);
        break;
    case MESHX_NOTIFY_TYPE_BEACON:
        if (meshx_show_beacon)
        {
            meshx_notify_beacon_cb(pdata, len);
        }
        break;
    default:
        MESHX_ERROR("unknown notify type: %d", notify_type);
        break;
    }

    return MESHX_SUCCESS;
}

static int32_t meshx_async_msg_notify_handler(void)
{
    async_data_t async_data;
    async_data.type = ASYNC_DATA_TYPE_MESH_DATA;
    msg_queue_send(msg_queue, &async_data, sizeof(async_data_t));

    return MESHX_SUCCESS;
}

static void *meshx_thread(void *pargs)
{
    msg_queue_init();
    msg_queue_create(&msg_queue, 10, sizeof(async_data_t));
    meshx_async_msg_init(10, meshx_async_msg_notify_handler);


    meshx_trace_init();
    meshx_trace_level_enable(MESHX_TRACE_LEVEL_ALL);

    meshx_notify_init(meshx_notify_cb);

    meshx_gap_init();
    meshx_bearer_init();
    meshx_network_init();
    meshx_provision_init();

    meshx_gap_start();
    meshx_bearer_param_t adv_param = {.bearer_type = MESHX_BEARER_TYPE_ADV, .param_adv.adv_period = 0};
    adv_bearer = meshx_bearer_create(adv_param);

    meshx_network_if_t adv_network_if = meshx_network_if_create();
    meshx_network_if_connect(adv_network_if, adv_bearer, NULL, NULL);

#if 0
    meshx_dev_uuid_t dev_uuid = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    meshx_provision_dev_t prov_dev = meshx_provision_create_device(adv_bearer, dev_uuid);
    meshx_provision_link_open(prov_dev);
#endif

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
                    meshx_async_msg_process();
                }
                break;
            case ASYNC_DATA_TYPE_TTY:
                {
                    meshx_cmd_collect(async_data.data, 1);
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
    uint8_t adv_recv_data[32];

    while (1)
    {
        data_len = read(fd_dspr, adv_recv_data, 31);
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
    int32_t data = 0;

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

    pthread_create(&meshx_tid, NULL, meshx_thread, NULL);
    pthread_create(&meshx_receive, NULL, meshx_receive_thread, NULL);
    pthread_create(&console_receive, NULL, console_receive_thread, NULL);
    pthread_join(meshx_tid, NULL);
    pthread_join(meshx_receive, NULL);
    pthread_join(console_receive, NULL);

    return 0;
}
