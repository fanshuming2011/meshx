/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "PROVISIONER"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "meshx.h"
//#include "meshx_mem.h"

#define FIFO_DSPR  "/tmp/fifo_dspr"
#define FIFO_PSDR  "/tmp/fifo_psdr"


static pthread_t meshx_tid;
static pthread_t meshx_receive;
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
}

void linux_send_string(const char *pdata, uint32_t len)
{
    fwrite(pdata, 1, len, log_file);
    fflush(log_file);
}

static void *meshx_thread(void *pargs)
{
    meshx_trace_init(linux_send_string);
    meshx_trace_level_enable(MESHX_TRACE_LEVEL_ALL);
    meshx_gap_init();
    meshx_bearer_init();
    meshx_network_init();
    meshx_provision_init();

    meshx_gap_start();
    meshx_bearer_param_t adv_param = {.bearer_type = MESHX_BEARER_TYPE_ADV, .param_adv.adv_period = 0};
    meshx_bearer_t adv_bearer = meshx_bearer_create(adv_param);

    meshx_network_if_t adv_network_if = meshx_network_if_create(MESHX_NETWORK_IF_TYPE_ADV);
    meshx_network_if_connect(adv_network_if, adv_bearer, NULL, NULL);

    meshx_dev_uuid_t dev_uuid = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    meshx_provision_link_open(adv_bearer, dev_uuid);

    //uint8_t test_data[] = {1, 2, 3, 4, 5, 6};

    while (1)
    {
        //write(fd_psdr, test_data, 6);
        sleep(1);
    }
    pthread_exit((void *)0);
}

static void *meshx_receive_thread(void *pargs)
{
    int data_len = 0;
    uint8_t adv_recv_data[64];
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
        data_len = read(fd_dspr, adv_recv_data, 31);
        /* data read finished */
        MESHX_DUMP_DEBUG(adv_recv_data, data_len);
        meshx_gap_handle_adv_report(adv_recv_data, data_len, &rx_metadata);
    }
    pthread_exit((void *)0);
}

int main(int argc, char **argv)
{
    system_init();
    printf("provisioner system initialized success!\r\n");

    pthread_create(&meshx_tid, NULL, meshx_thread, NULL);
    pthread_create(&meshx_receive, NULL, meshx_receive_thread, NULL);
    pthread_join(meshx_tid, NULL);
    pthread_join(meshx_receive, NULL);

    return 0;
}
