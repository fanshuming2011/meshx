/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define MESHX_TRACE_MODULE "DEVICE"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
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
#include "device_cmd.h"
#include "meshx_cmd_prov.h"
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
    struct timeval tv_seed;
    gettimeofday(&tv_seed, NULL);
    meshx_srand(tv_seed.tv_usec);

    mkfifo(FIFO_DSPR, 0777);
    mkfifo(FIFO_PSDR, 0777);
    fd_psdr = open(FIFO_PSDR, O_RDONLY);
    fd_dspr = open(FIFO_DSPR, O_WRONLY);
    log_file = fopen("./log_dev", "w");
    meshx_tty_init();
    device_cmd_init();
    /* add keys */
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
            const meshx_prov_link_open_result_t *presult = pprov->pdata;
            meshx_tty_printf("link opened, result: %d\r\n", *presult);
        }
        break;
    case MESHX_PROV_NOTIFY_LINK_CLOSE:
        {
            const uint8_t *preason = pprov->pdata;
            meshx_tty_printf("link closed, reason: %d\r\n", *preason);
        }
        break;
    case MESHX_PROV_NOTIFY_INVITE:
        {
            const meshx_prov_invite_t *pinvite = pprov->pdata;
            meshx_tty_printf("invite: %d\r\n", pinvite->attention_duration);
            /* send capabilites */
            meshx_prov_capabilites_t cap;
            memset(&cap, 0, sizeof(meshx_prov_capabilites_t));
            cap.algorithms = MESHX_PROV_CAP_ALGORITHM_P256_CURVE;
            cap.element_nums = 1;
            meshx_prov_capabilites(pprov->metadata.prov_dev, &cap);
        }
        break;
    case MESHX_PROV_NOTIFY_START:
        {
            const meshx_prov_start_t *pstart = pprov->pdata;
            meshx_tty_printf("start:");
            meshx_tty_dump((const uint8_t *)pstart, sizeof(meshx_prov_start_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_PUBLIC_KEY:
        {
            const meshx_prov_public_key_t *ppub_key = pprov->pdata;
            meshx_tty_printf("public key:");
            meshx_tty_dump((const uint8_t *)ppub_key, sizeof(meshx_prov_public_key_t));
            meshx_tty_printf("\r\n");

            /* send public key */
            meshx_tty_printf("send public key\r\n");
            meshx_prov_public_key_t pub_key;
            meshx_prov_get_local_public_key(pprov->metadata.prov_dev, &pub_key);
            meshx_prov_public_key(pprov->metadata.prov_dev, &pub_key);

            /* generate auth value */
            meshx_prov_auth_value_t auth_value;
            auth_value.auth_method = MESHX_PROV_AUTH_METHOD_NO_OOB;
            meshx_prov_set_auth_value(pprov->metadata.prov_dev, &auth_value);
            /* generate confirmation */
            meshx_prov_random_t random;
            meshx_prov_get_random(pprov->metadata.prov_dev, &random);
            meshx_prov_generate_confirmation(pprov->metadata.prov_dev, &random);
        }
        break;
    case MESHX_PROV_NOTIFY_CONFIRMATION:
        {
            const meshx_prov_confirmation_t *pcfm = pprov->pdata;
            meshx_tty_printf("confirmation:");
            meshx_tty_dump((const uint8_t *)pcfm, sizeof(meshx_prov_confirmation_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_RANDOM:
        {
            const meshx_prov_random_t *prandom = pprov->pdata;
            meshx_tty_printf("random:");
            meshx_tty_dump((const uint8_t *)prandom, sizeof(meshx_prov_random_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_DATA:
        {
            const meshx_prov_data_t *pdata = pprov->pdata;
            meshx_tty_printf("data:");
            meshx_tty_dump((const uint8_t *)pdata, sizeof(meshx_prov_data_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_TRANS_ACK:
        {
            const meshx_prov_state_t *pstate = pprov->pdata;
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
    uint8_t prov_id = meshx_prov_get_device_id(pprov->metadata.prov_dev);

    switch (pprov->metadata.notify_type)
    {
    case MESHX_PROV_NOTIFY_LINK_OPEN:
        {
            meshx_cmd_prov_add_device(pprov->metadata.prov_dev);
            const meshx_prov_link_open_result_t *presult = pprov->pdata;
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
    case MESHX_PROV_NOTIFY_INVITE:
        {
            const meshx_prov_invite_t *pinvite = pprov->pdata;
            meshx_tty_printf("invite: id %d, attention duration %d\r\n", prov_id, pinvite->attention_duration);
        }
        break;
    case MESHX_PROV_NOTIFY_START:
        {
            const meshx_prov_start_t *pstart = pprov->pdata;
            meshx_tty_printf("start: id %d, algorithm %d, public key %d, auth method %d, auth_aciton %d, auth size %d\r\n",
                             prov_id, pstart->algorithm, pstart->public_key, pstart->auth_method, pstart->auth_action,
                             pstart->auth_size);
            if (MESHX_PROV_PUBLIC_KEY_OOB == pstart->public_key)
            {
                /* dump public key */
                meshx_tty_printf("my public key:");
                meshx_prov_public_key_t pub_key;
                meshx_prov_get_local_public_key(pprov->metadata.prov_dev, &pub_key);
                meshx_tty_dump((const uint8_t *)&pub_key, sizeof(meshx_prov_public_key_t));
                meshx_tty_printf("\r\n");
            }
        }
        break;
    case MESHX_PROV_NOTIFY_PUBLIC_KEY:
        {
            const meshx_prov_public_key_t *ppub_key = pprov->pdata;
            meshx_tty_printf("public key:");
            meshx_tty_dump((const uint8_t *)ppub_key, sizeof(meshx_prov_public_key_t));
            meshx_tty_printf("\r\n");

            if (MESHX_PROV_AUTH_METHOD_OUTPUT_OOB == meshx_prov_get_auth_method(
                    pprov->metadata.prov_dev))
            {
                switch (meshx_prov_get_auth_action(pprov->metadata.prov_dev))
                {
                case MESHX_PROV_AUTH_ACTION_BLINK:
                case MESHX_PROV_AUTH_ACTION_BEEP:
                case MESHX_PROV_AUTH_ACTION_VIBRATE:
                    meshx_tty_printf("auth value numeric: 5\r\n");
                    break;
                case MESHX_PROV_AUTH_ACTION_OUTPUT_NUMERIC:
                    meshx_tty_printf("auth value numeric: 019655\r\n");
                    break;
                case MESHX_PROV_AUTH_ACTION_OUTPUT_ALPHA:
                    meshx_tty_printf("auth value alpha: 123ABC\r\n");
                    break;
                default:
                    break;
                }
            }
        }
        break;
    case MESHX_PROV_NOTIFY_CONFIRMATION:
        {
            const meshx_prov_confirmation_t *pcfm = pprov->pdata;
            meshx_tty_printf("confirmation:");
            meshx_tty_dump((const uint8_t *)pcfm, sizeof(meshx_prov_confirmation_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_RANDOM:
        {
            const meshx_prov_random_t *prandom = pprov->pdata;
            meshx_tty_printf("random:");
            meshx_tty_dump((const uint8_t *)prandom, sizeof(meshx_prov_random_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_DATA:
        {
            const meshx_prov_data_t *pdata = pprov->pdata;
            meshx_tty_printf("data:");
            meshx_tty_dump((const uint8_t *)pdata, sizeof(meshx_prov_data_t));
            meshx_tty_printf("\r\n");
        }
        break;
    case MESHX_PROV_NOTIFY_TRANS_ACK:
        {
            const meshx_prov_state_t *pstate = pprov->pdata;
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
    default:
        meshx_tty_printf("unknown provision type: %d\r\n", pprov->metadata.notify_type);
        break;
    }
    return MESHX_SUCCESS;
}
#endif

static int32_t meshx_notify_beacon_cb(const void *pdata, uint8_t len)
{
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
        meshx_notify_beacon_cb(pdata, len);
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

static void meshx_dev_cfg(void)
{
    meshx_node_config_t config;
    meshx_node_config_init(&config);
    meshx_node_config_set(&config);

    meshx_node_param_t param;
    meshx_node_params_init(&param);
    param.node_addr = 0x1201;
    meshx_node_params_set(&param);
    meshx_iv_index_set(0x12345678);
}

static void meshx_dev_init(void)
{
    /* add keys */
    meshx_net_key_add(0, sample_net_key);
    meshx_app_key_add(0, 0, sample_app_key);
    meshx_dev_key_add(0x1201, 1, sample_dev_key);
}

static void *meshx_thread(void *pargs)
{
    msg_queue_init();
    msg_queue_create(&msg_queue, 10, sizeof(async_data_t));
    meshx_async_msg_init(10, meshx_async_msg_notify_handler);
    meshx_notify_init(meshx_notify_cb);

    /* TODO: mode to common? */
    meshx_trace_init();
    meshx_trace_level_enable(MESHX_TRACE_LEVEL_ALL);
    meshx_trace_level_disable(MESHX_TRACE_LEVEL_DEBUG);


    meshx_dev_cfg();

    /* init stack */
    meshx_init();

    meshx_dev_init();

    /* run stack */
    meshx_run();

    meshx_bearer_rx_metadata_t rx_metadata;
    meshx_bearer_rx_metadata_adv_t adv_metadata =
    {
        .adv_type = MESHX_GAP_ADV_TYPE_NONCONN_IND,
        .peer_addr = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
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

    pthread_create(&meshx_tid, NULL, meshx_thread, NULL);
    pthread_create(&meshx_receive, NULL, meshx_receive_thread, NULL);
    pthread_create(&console_receive, NULL, console_receive_thread, NULL);
    pthread_join(meshx_tid, NULL);
    pthread_join(meshx_receive, NULL);
    pthread_join(console_receive, NULL);

    return 0;
}

