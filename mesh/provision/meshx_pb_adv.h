/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_PB_ADV_H_
#define _MESHX_PB_ADV_H_

#include "meshx_bearer.h"
#include "meshx_provision.h"

MESHX_BEGIN_DECLS

#define MESHX_PB_ADV_PDU_MAX_LEN                24

#define MESHX_GPCF_TRANS_START                  0x00
#define MESHX_GPCF_TRANS_ACK                    0x01
#define MESHX_GPCF_TRANS_CONTINUE               0x02
#define MESHX_GPCF_BEARER_CTL                   0x03

#define MESHX_BEARER_LINK_OPEN                  0x00
#define MESHX_BEARER_LINK_ACK                   0x01
#define MESHX_BEARER_LINK_CLOSE                 0x02

#define MESHX_LINK_CLOSE_REASON_SUCCESS         0x00
#define MESHX_LINK_CLOSE_REASON_TIMEOUT         0x01
#define MESHX_LINK_CLOSE_REASON_FAIL            0x02

typedef struct
{
    uint32_t link_id;
    uint8_t trans_num;
} __PACKED meshx_pb_adv_metadata_t;

typedef struct
{
    uint8_t gpcf: 2;
    uint8_t seg_num: 6;
    uint16_t total_len;
    uint8_t fcs;
} __PACKED meshx_pb_adv_trans_start_metadata_t;


#define MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN       (MESHX_PB_ADV_PDU_MAX_LEN - sizeof(meshx_pb_adv_trans_start_metadata_t))
typedef struct
{
    meshx_pb_adv_trans_start_metadata_t metadata;
    uint8_t pdu[MESHX_PB_ADV_TRANS_START_PDU_MAX_LEN];
} __PACKED meshx_pb_adv_trans_start_t;

typedef struct
{
    uint8_t gpcf: 2;
    uint8_t padding : 8;
} meshx_pb_adv_trans_ack_metadata_t;

typedef struct
{
    meshx_pb_adv_trans_ack_metadata_t metadata;
} __PACKED meshx_pb_adv_trans_ack_t;

typedef struct
{
    uint8_t gpcf: 2;
    uint8_t seg_index: 6;
} __PACKED meshx_pb_adv_trans_continue_metadata_t;

#define MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN       (MESHX_PB_ADV_PDU_MAX_LEN - sizeof(meshx_pb_adv_trans_continue_metadata_t))
typedef struct
{
    meshx_pb_adv_trans_continue_metadata_t metadata;
    uint8_t pdu[MESHX_PB_ADV_TRANS_CONTINUE_PDU_MAX_LEN];
} __PACKED meshx_pb_adv_trans_continue_t;

typedef struct
{
    meshx_dev_uuid_t dev_uuid;
} __PACKED meshx_pb_adv_link_open_t;

typedef struct
{
} __PACKED meshx_pb_adv_link_ack_t;

typedef struct
{
    uint8_t reason;
} __PACKED meshx_pb_adv_link_close_t;

typedef struct
{
    uint8_t gpcf: 2;
    uint8_t bearer_opcode: 6;
} __PACKED meshx_pb_adv_bearer_ctl_metadata_t;

typedef struct
{
    meshx_pb_adv_bearer_ctl_metadata_t metadata;
    union
    {
        meshx_pb_adv_link_open_t link_open;
        meshx_pb_adv_link_ack_t link_ack;
        meshx_pb_adv_link_close_t link_close;
    };
} __PACKED meshx_pb_adv_bearer_ctl_t;

typedef struct
{
    uint8_t gpcf: 2;
    uint8_t padding: 6;
    uint8_t pdu[MESHX_PB_ADV_PDU_MAX_LEN - 1];
} __PACKED meshx_pb_adv_pdu_t;

typedef struct
{
    meshx_pb_adv_metadata_t metadata;
    union
    {
        meshx_pb_adv_trans_start_t trans_start;
        meshx_pb_adv_trans_ack_t trans_ack;
        meshx_pb_adv_trans_continue_t trans_continue;
        meshx_pb_adv_bearer_ctl_t bearer_ctl;
        meshx_pb_adv_pdu_t pdu;
    };
} __PACKED meshx_pb_adv_pkt_t;


MESHX_EXTERN int32_t meshx_pb_adv_init(void);
MESHX_EXTERN int32_t meshx_pb_adv_link_open(meshx_bearer_t bearer, uint32_t link_id,
                                            meshx_dev_uuid_t dev_uuid);
MESHX_EXTERN int32_t meshx_pb_adv_link_ack(meshx_bearer_t bearer, uint32_t link_id);
MESHX_EXTERN int32_t meshx_pb_adv_link_close(meshx_bearer_t bearer, uint32_t link_id,
                                             uint8_t reason);
MESHX_EXTERN int32_t meshx_pb_adv_receive(meshx_bearer_t bearer, const uint8_t *pdata, uint8_t len);

MESHX_EXTERN int32_t meshx_pb_adv_invite(meshx_bearer_t bearer, uint32_t link_id,
                                         meshx_provision_invite_t invite);

MESHX_END_DECLS

#endif /* _MESHX_PB_ADV_H_ */