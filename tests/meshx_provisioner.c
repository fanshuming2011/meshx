/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "PROVISIONER"
#include <time.h>
#include <sys/time.h>
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
#include "meshx_cmd_prov.h"
//#include "meshx_mem.h"


static struct timeval tv_prov_begin;
static struct timeval tv_prov_end;
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
    struct timeval tv_seed;
    gettimeofday(&tv_seed, NULL);
    meshx_srand(tv_seed.tv_usec);

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

#if 1
static int32_t meshx_notify_prov_cb(const void *pdata, uint8_t len)
{
    const meshx_notify_prov_t *pprov = pdata;

    switch (pprov->metadata.notify_type)
    {
    case MESHX_PROV_NOTIFY_LINK_OPEN:
        {
            gettimeofday(&tv_prov_begin, NULL);
            const meshx_provision_link_open_result_t *presult = pprov->pdata;
            meshx_tty_printf("link opened, result: %d\r\n", *presult);
            if (*presult == MESHX_PROVISION_LINK_OPEN_SUCCESS)
            {
                meshx_provision_invite_t invite = {0};
                /* send invite */
                meshx_provision_invite(pprov->metadata.prov_dev, invite);
            }
        }
        break;
    case MESHX_PROV_NOTIFY_LINK_CLOSE:
        {
            const uint8_t *preason = pprov->pdata;
            meshx_tty_printf("link closed, reason: %d\r\n", *preason);
        }
        break;
    case MESHX_PROV_NOTIFY_CAPABILITES:
        {
            const meshx_provision_capabilites_t *pcap = pprov->pdata;
            meshx_tty_printf("capabilites:");
            meshx_tty_dump((const uint8_t *)pcap, sizeof(meshx_provision_capabilites_t));
            meshx_tty_printf("\r\n");
            /* send start */
            meshx_provision_start_t start;
            memset(&start, 0, sizeof(meshx_provision_start_t));
            start.algorithm = MESHX_PROVISION_ALGORITHM_P256_CURVE;
            start.auth_method = MESHX_PROVISION_AUTH_METHOD_NO_OOB;
            meshx_provision_start(pprov->metadata.prov_dev, &start);
        }
        break;
    case MESHX_PROV_NOTIFY_PUBLIC_KEY:
        {
            const meshx_provision_public_key_t *ppub_key = pprov->pdata;
            meshx_tty_printf("public key:");
            meshx_tty_dump((const uint8_t *)ppub_key, sizeof(meshx_provision_public_key_t));
            meshx_tty_printf("\r\n");

            /* generate auth value */
            meshx_provision_auth_value_t auth_value;
            auth_value.auth_method = MESHX_PROVISION_AUTH_METHOD_NO_OOB;
            meshx_provision_set_auth_value(pprov->metadata.prov_dev, &auth_value);
            /* generate confirmation */
            meshx_provision_random_t random;
            meshx_provision_get_random(pprov->metadata.prov_dev, &random);
            meshx_provision_generate_confirmation(pprov->metadata.prov_dev, &random);

            /* send confirmation */
            meshx_provision_confirmation_t cfm;
            meshx_provision_get_confirmation(pprov->metadata.prov_dev, &cfm);
            meshx_provision_confirmation(pprov->metadata.prov_dev, &cfm);
        }
        break;
    case MESHX_PROV_NOTIFY_CONFIRMATION:
        {
            const meshx_provision_confirmation_t *pcfm = pprov->pdata;
            meshx_tty_printf("confirmation:");
            meshx_tty_dump((const uint8_t *)pcfm, sizeof(meshx_provision_confirmation_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_RANDOM:
        {
            const meshx_provision_random_t *prandom = pprov->pdata;
            meshx_tty_printf("random:");
            meshx_tty_dump((const uint8_t *)prandom, sizeof(meshx_provision_random_t));
            meshx_tty_printf("\r\n");

            /* send data */
            meshx_provision_data_t data;
            memset(&data, 0, sizeof(meshx_provision_data_t));
            meshx_provision_data(pprov->metadata.prov_dev, &data);
        }
        break;
    case MESHX_PROV_NOTIFY_TRANS_ACK:
        {
            const meshx_provision_state_t *pstate = pprov->pdata;
            meshx_tty_printf("ack: %d\r\n", *pstate);
            if (MESHX_PROVISION_STATE_START == *pstate)
            {
                meshx_tty_printf("send public key\r\n");

                /* send public key */
                meshx_provision_public_key_t pub_key;
                meshx_provision_get_local_public_key(pprov->metadata.prov_dev, &pub_key);
                meshx_provision_public_key(pprov->metadata.prov_dev, &pub_key);
            }
        }
        break;
    case MESHX_PROV_NOTIFY_FAILED:
        {
            /* @ref meshx provisison failed error code macros */
            uint8_t err_code = *((const uint8_t *)pprov->pdata);
            meshx_tty_printf("provision failed: %d\r\n", err_code);
        }
        break;
    case MESHX_PROV_NOTIFY_COMPLETE:
        {
            meshx_tty_printf("provision complete\r\n");

            gettimeofday(&tv_prov_end, NULL);
            uint32_t time = (tv_prov_end.tv_sec - tv_prov_begin.tv_sec) * 1000;
            if (tv_prov_end.tv_usec < tv_prov_begin.tv_usec)
            {
                time -= ((tv_prov_begin.tv_usec - tv_prov_end.tv_usec) / 1000);
            }
            else
            {
                time += ((tv_prov_end.tv_usec - tv_prov_begin.tv_usec) / 1000);
            }

            meshx_tty_printf("prov time: %dms\r\n", time);
        }
        break;
    default:
        meshx_tty_printf("unknown provision type: %d\r\n", pprov->metadata.notify_type);
        break;
    }
    return MESHX_SUCCESS;
}
#else
static int32_t meshx_notify_prov_cb(const void *pdata, uint8_t len)
{
    const meshx_notify_prov_t *pprov = pdata;
    uint8_t prov_id = meshx_provision_get_device_id(pprov->metadata.prov_dev);

    switch (pprov->metadata.notify_type)
    {
    case MESHX_PROV_NOTIFY_LINK_OPEN:
        {
            meshx_cmd_prov_add_device(pprov->metadata.prov_dev);
            gettimeofday(&tv_prov_begin, NULL);
            const meshx_provision_link_open_result_t *presult = pprov->pdata;
            meshx_tty_printf("link opened: result %d, id %d\r\n", *presult, prov_id);
        }
        break;
    case MESHX_PROV_NOTIFY_LINK_CLOSE:
        {
            meshx_cmd_prov_remove_device(pprov->metadata.prov_dev);
            const uint8_t *preason = pprov->pdata;
            meshx_tty_printf("link closed: reason %d, id %d\r\n", *preason, prov_id);
        }
        break;
    case MESHX_PROV_NOTIFY_CAPABILITES:
        {
            const meshx_provision_capabilites_t *pcap = pprov->pdata;
            meshx_tty_printf("capabilites: id %d, element_nums %d, algorithm %d, public key type %d, static oob type %d, output oob size %d, output oob action %d, input oob size %d, input oob action %d\r\n",
                             prov_id, pcap->element_nums, pcap->algorithms, pcap->public_key_type, pcap->static_oob_type,
                             pcap->output_oob_size, pcap->output_oob_action, pcap->input_oob_size, pcap->input_oob_action);
        }
        break;
    case MESHX_PROV_NOTIFY_PUBLIC_KEY:
        {
            const meshx_provision_public_key_t *ppub_key = pprov->pdata;
            meshx_tty_printf("public key:");
            meshx_tty_dump((const uint8_t *)ppub_key, sizeof(meshx_provision_public_key_t));
            meshx_tty_printf("\r\n");

            if (MESHX_PROVISION_AUTH_METHOD_INPUT_OOB == meshx_provision_get_auth_method(
                    pprov->metadata.prov_dev))
            {
                switch (meshx_provision_get_auth_action(pprov->metadata.prov_dev))
                {
                case MESHX_PROVISION_AUTH_ACTION_PUSH:
                case MESHX_PROVISION_AUTH_ACTION_TWIST:
                    meshx_tty_printf("auth value numeric: 5\r\n");
                    break;
                case MESHX_PROVISION_AUTH_ACTION_INPUT_NUMERIC:
                    meshx_tty_printf("auth value numeric: 019655\r\n");
                    break;
                case MESHX_PROVISION_AUTH_ACTION_INPUT_ALPHA:
                    meshx_tty_printf("auth value alpha: 123ABC\r\n");
                    break;
                default:
                    break;
                }
            }
        }
        break;
    case MESHX_PROV_NOTIFY_INPUT_COMPLETE:
        meshx_tty_printf("input complete\r\n");
        break;
    case MESHX_PROV_NOTIFY_CONFIRMATION:
        {
            const meshx_provision_confirmation_t *pcfm = pprov->pdata;
            meshx_tty_printf("confirmation:");
            meshx_tty_dump((const uint8_t *)pcfm, sizeof(meshx_provision_confirmation_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_RANDOM:
        {
            const meshx_provision_random_t *prandom = pprov->pdata;
            meshx_tty_printf("random:");
            meshx_tty_dump((const uint8_t *)prandom, sizeof(meshx_provision_random_t));
            meshx_tty_printf("\r\n");

            /* send data */
            meshx_provision_data_t data;
            memset(&data, 0, sizeof(meshx_provision_data_t));
            meshx_provision_data(pprov->metadata.prov_dev, &data);
        }
        break;
    case MESHX_PROV_NOTIFY_TRANS_ACK:
        {
            const meshx_provision_state_t *pstate = pprov->pdata;
            meshx_tty_printf("ack: %d\r\n", *pstate);
        }
        break;
    case MESHX_PROV_NOTIFY_FAILED:
        {
            /* @ref meshx provisison failed error code macros */
            uint8_t err_code = *((const uint8_t *)pprov->pdata);
            meshx_tty_printf("provision failed: %d\r\n", err_code);
        }
        break;
    case MESHX_PROV_NOTIFY_COMPLETE:
        {
            meshx_tty_printf("provision complete\r\n");

            gettimeofday(&tv_prov_end, NULL);
            uint32_t time = (tv_prov_end.tv_sec - tv_prov_begin.tv_sec) * 1000;
            if (tv_prov_end.tv_usec < tv_prov_begin.tv_usec)
            {
                time -= ((tv_prov_begin.tv_usec - tv_prov_end.tv_usec) / 1000);
            }
            else
            {
                time += ((tv_prov_end.tv_usec - tv_prov_begin.tv_usec) / 1000);
            }

            meshx_tty_printf("prov time: %dms\r\n", time);
        }
        break;
    default:
        meshx_tty_printf("unknown provision type: %d\r\n", pprov->metadata.notify_type);
        break;
    }
    return MESHX_SUCCESS;
}
#endif

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

static int32_t meshx_notify_cb(meshx_bearer_t bearer, uint8_t notify_type, const void *pdata,
                               uint8_t len)
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

static void meshx_prov_cfg(void)
{
    meshx_node_config_t config;
    meshx_node_config_init(&config);
    config.role = MESHX_ROLE_PROVISIONER;
    meshx_node_config_set(&config);

    meshx_node_param_t param;
    meshx_node_params_init(&param);
    param.node_addr = 0x03;
    meshx_node_params_set(&param);

    meshx_iv_index_set(0x12345678);
}

static void meshx_prov_init(void)
{
    /* add keys */
    meshx_net_key_add(0, sample_net_key);
    meshx_app_key_add(0, 0, sample_app_key);
}

static void *meshx_thread(void *pargs)
{
    msg_queue_init();
    msg_queue_create(&msg_queue, 10, sizeof(async_data_t));
    meshx_async_msg_init(10, meshx_async_msg_notify_handler);
    meshx_notify_init(meshx_notify_cb);

    meshx_trace_init();
    meshx_trace_level_enable(MESHX_TRACE_LEVEL_ALL);
    //meshx_trace_level_disable(MESHX_TRACE_LEVEL_DEBUG);

    meshx_prov_cfg();

    /* init stack */
    meshx_init();

    meshx_prov_init();

    /* run stack */
    meshx_run();

    /*************** initialize sample data ****************/
    meshx_bearer_rx_metadata_t rx_metadata;
    rx_metadata.bearer_type = MESHX_BEARER_TYPE_ADV;
    meshx_bearer_t adv_bearer = meshx_bearer_get(&rx_metadata);
    meshx_network_if_t adv_net_if = meshx_network_if_get(adv_bearer);

    /*********************** send sample network data *********************/
    meshx_msg_ctx_t ctx;
#if 0
    ctx.ctl = 0x01;
    ctx.dst = 0xfffd;
    ctx.element_index = 0;
    ctx.ttl = 0;
    ctx.seq = 1;
    ctx.pnet_key = meshx_net_key_get(0);
    uint8_t trans_pdu[] = {0x03, 0x4b, 0x50, 0x05, 0x7e, 0x40, 0x00, 0x00, 0x01, 0x00, 0x00};
    meshx_network_send(adv_net_if, trans_pdu, sizeof(trans_pdu), &ctx);
#endif
    /*******************************************************/

    /*********************** send sample lower transport data *********************/
    const meshx_application_key_t *papp_key = meshx_app_key_get(0);
    meshx_seq_set(0, 0x3129ab);
    ctx.ctl = 0x00;
    ctx.src = meshx_node_params_get().node_addr;
    ctx.dst = 0x1201;
    ctx.ttl = 4;
    ctx.seq = 0x3129ab;
    ctx.pnet_key = meshx_net_key_get(0);
    ctx.papp_key = &papp_key->app_key;
    ctx.aid = papp_key->aid;
    ctx.akf = 1;
    ctx.szmic = 0;
    ctx.seq_origin = 0x3129ab;
    uint8_t upper_trans_pdu[] = {0xee, 0x9d, 0xdd, 0xfd, 0x21, 0x69, 0x32, 0x6d, 0x23, 0xf3, 0xaf, 0xdf, 0xcf, 0xdc, 0x18, 0xc5, 0x2f, 0xde, 0xf7, 0x72, 0xe0, 0xe1, 0x73, 0x08};
    meshx_lower_transport_send(adv_net_if, upper_trans_pdu, sizeof(upper_trans_pdu), &ctx);
    /*******************************************************/

    //meshx_bearer_rx_metadata_t rx_metadata;
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
