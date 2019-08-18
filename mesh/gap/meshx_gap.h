/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_GAP_H_
#define _MESHX_GAP_H_

#include "meshx_common.h"
#include "meshx_gap_wrapper.h"

MESHX_BEGIN_DECLS

#define MESHX_GAP_ADV_DATA_MAX_LEN               31
#define MESHX_GAP_ADV_PDU_MAX_LEN                29

#define MESHX_GAP_ADTYPE_PB_ADV                  0x29
#define MESHX_GAP_ADTYPE_MESH_MSG                0x2A
#define MESHX_GAP_ADTYPE_MESH_BEACON             0x2B

#define MESHX_GAP_ADV_LENGTH_FIELD_SIZE          1
#define MESHX_GAP_ADV_PDU_OFFSET                 2
#define MESHX_GAP_GET_ADV_PDU(pdata)             (pdata + MESHX_GAP_ADV_PDU_OFFSET)
#define MESHX_GAP_GET_ADV_PDU_LEN(pdata)         (pdata[0] - 1)
#define MESHX_GAP_GET_ADV_LEN(pdata)             (pdata[0])
#define MESHX_GAP_GET_ADV_TYPE(pdata)            (pdata[1])

typedef enum
{
    MESHX_BT_STATUS_STACK_STATE_CHANGE,
    MESHX_BT_STATUS_SCAN_STATE_CHANGE,
    MESHX_BT_STATUS_ADV_STATE_CHANGE,
    MESHX_BT_STATUS_CONN_STATE_CHANGE
} meshx_gap_bt_status_type_t;

#define MESHX_GAP_STACK_STATE_INIT              0
#define MESHX_GAP_STACK_STATE_READY             1

#define MESHX_GAP_ADV_STATE_IDLE                0
#define MESHX_GAP_ADV_STATE_ADVERTISING         1

#define MESHX_GAP_SCAN_STATE_IDLE               0
#define MESHX_GAP_SCAN_STATE_SCANING            1

#define MESHX_GAP_CONN_STATE_CONNECTED  　　　　　0
#define MESHX_GAP_CONN_STATE_DISCONNECTED       1
typedef struct
{
    uint8_t conn_state;
    uint8_t conn_id;
    uint8_t disc_reason;
} meshx_gap_conn_state_t;


typedef struct
{
    meshx_gap_bt_status_type_t status_type;
    union
    {
        uint8_t stack_state;
        uint8_t adv_state;
        uint8_t scan_state;
        meshx_gap_conn_state_t conn_state;
    };
} meshx_gap_bt_status_t;

typedef enum
{
    MESHX_GAP_ACTION_TYPE_SCAN,
    MESHX_GAP_ACTION_TYPE_ADV
} meshx_gap_action_type_t;

typedef struct
{
    meshx_gap_scan_type_t scan_type;
    uint16_t scan_window;
    uint16_t scan_interval;
} meshx_gap_action_data_scan_t;

typedef struct
{
    meshx_gap_adv_type_t adv_type;
    uint8_t data[MESHX_GAP_ADV_DATA_MAX_LEN];
    uint8_t data_len;
} meshx_gap_action_data_adv_t;

typedef struct
{
    meshx_gap_action_type_t action_type;
    union
    {
        meshx_gap_action_data_adv_t action_adv_data;
        meshx_gap_action_data_scan_t action_scan_data;
    };
} meshx_gap_action_t;;

MESHX_EXTERN int32_t meshx_gap_init(uint8_t gap_task_num);
MESHX_EXTERN int32_t meshx_gap_start(void);
MESHX_EXTERN void meshx_gap_stop(void);
MESHX_EXTERN int32_t meshx_gap_handle_bt_status(const meshx_gap_bt_status_t *pstatus);
MESHX_EXTERN int32_t meshx_gap_add_action(const meshx_gap_action_t *paction);
MESHX_EXTERN int32_t meshx_gap_handle_adv_report(const uint8_t *pdata, uint16_t len,
                                                 const meshx_bearer_rx_metadata_t *prx_metadata);
MESHX_EXTERN void meshx_gap_adv_done(void);


MESHX_END_DECLS


#endif /* _MESHX_GAP_H_ */


