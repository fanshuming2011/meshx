/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_GAP_WRAPPER_H_
#define _MESHX_GAP_WRAPPER_H_

MESHX_BEGIN_DECLS

typedef enum
{
    MESHX_GAP_PUBLIC_ADDR,
    MESHX_GAP_RANDOM_ADDR,
} meshx_mac_addr_type_t;

#define MESHX_MAC_ADDR_LEN                           6
typedef uint8_t meshx_mac_addr_t[MESHX_MAC_ADDR_LEN];

typedef enum
{
    MESHX_GAP_SCAN_TYPE_PASSIVE,
    MESHX_GAP_SCAN_TYPE_ACTIVE,
} meshx_gap_scan_type_t;

typedef struct
{
    meshx_gap_scan_type_t scan_type;
    uint16_t scan_interval;  /* Range: 0x0004 to 0x4000 Time = N * 0.625 msec Time Range: 2.5 msec to 10.24 seconds */
    uint16_t scan_window;    /* Range: 0x0004 to 0x4000 Time = N * 0.625 msec Time Range: 2.5 msec to 10.24 seconds */
} meshx_gap_scan_param_t;

typedef enum
{
    MESHX_GAP_ADV_TYPE_IND,               /* connectable and scannable undiredted advertising */
    MESHX_GAP_ADV_TYPE_DIRECT_IND,        /* connectable high duty cycle directed advertising */
    MESHX_GAP_ADV_TYPE_SCAN_IND,          /* scannable undirected advertising */
    MESHX_GAP_ADV_TYPE_NONCONN_IND,       /* non connectable undirected advertising */
    MESHX_GAP_ADV_TYPE_DIRECTED_IND,      /* connectable low duty cycle directed advertising */
} meshx_gap_adv_type_t;


#define MESHX_GAP_CHANNEL_37      0x01
#define MESHX_GAP_CHANNEL_38      0x02
#define MESHX_GAP_CHANNEL_39      0x04
#define MESHX_GAP_CHANNEL_ALL     0x07

typedef struct
{
    uint16_t adv_interval_min;    /* Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 seconds */
    uint16_t adv_interval_max;    /* Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 seconds */
    meshx_gap_adv_type_t adv_type;
    uint8_t channel;
} meshx_gap_adv_param_t;

int32_t meshx_gap_get_mac_addr(meshx_mac_addr_t mac_addr);

int32_t meshx_gap_scan_set_param(const meshx_gap_scan_param_t *pscan_param);
int32_t meshx_gap_scan_start(void);
int32_t meshx_gap_scan_stop(void);

int32_t meshx_gap_adv_set_param(const meshx_gap_adv_param_t *padv_param);
int32_t meshx_gap_adv_set_data(const uint8_t *pdata, uint8_t len);
int32_t meshx_gap_adv_set_scan_rsp_data(const uint8_t *pdata, uint8_t len);
int32_t meshx_gap_adv_start(void);
int32_t meshx_gap_adv_stop(void);

#define MESHX_GAP_GATT_TYPE_NOTIFY                            0
#define MESHX_GAP_GATT_TYPE_INDICATE                          1

int32_t meshx_gap_gatts_notify_or_indicate(uint16_t conn_handle, uint16_t srv_handle,
                                           uint16_t char_value_handle,
                                           uint8_t *p_value, uint8_t len, uint8_t type);

MESHX_END_DECLS

#endif /* _MESHX_GAP_WRAPPER_H_ */
