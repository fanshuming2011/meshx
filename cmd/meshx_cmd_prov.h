/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_CMD_PROV_H_
#define _MESHX_CMD_PROV_H_

#include "meshx_cmd.h"
#include "meshx_provision.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_cmd_prov_add_device(meshx_provision_dev_t prov_dev);
MESHX_EXTERN void meshx_cmd_prov_remove_device(meshx_provision_dev_t prov_dev);

MESHX_EXTERN int32_t meshx_cmd_prov_scan(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_prov_conn(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_prov_invite(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_prov_capabilites(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_prov_start(const meshx_cmd_parsed_data_t *pparsed_data);


#define MESHX_CMD_PROV \
    {\
        "prov_scan",\
        "prov_scan [state]",\
        "scan unprovisioned beacon, state: 0-hide udb 1-show udb",\
        meshx_cmd_prov_scan,\
    },\
    {\
        "prov_conn",\
        "prov_conn [uuid]",\
        "connect unprovisioned device use pb-adv, uuid is plaintext in hex format",\
        meshx_cmd_prov_conn,\
    },\
    {\
        "prov_invite",\
        "prov_invite [prov id] [attention duration]",\
        "send provision invite pdu.\r\n  attention duration: 0-255",\
        meshx_cmd_prov_invite,\
    },\
    {\
        "prov_capabilites",\
        "prov_capabilites [prov id] [element nums] [public key type] [static oob type] [output oob size] [output oob action] [input oob size] [input oob action]",\
        "send provision capabilites pdu.\r\n  element nums: 1-255\r\n  public key type: 0(no oob) 1(oob)\r\n  static oob type: 0(not available) 1(available)\r\n  output oob size: 0(not support) 1-8(maximum oob size) 9-255(RFU)\r\n  output oob action: bit0(blink) bit1(beep) bit2(vibrate) bit3(output numeric) bit4(output alpha) bit5-bit15(RFU)\r\n  input oob size: 0(not support) 1-8(maximum oob size) 9-255(RFU)\r\n  input oob action: bit0(push) bit1(twist) bit2(input numeric) bit3(input alpha) bit4-bit15(RFU)",\
        meshx_cmd_prov_capabilites,\
    },\
    {\
        "prov_start",\
        "prov_start [prov_id] [algorithm] [public key] [auth method] [auth action] [auth size]",\
        "provision start",\
        meshx_cmd_prov_start,\
    }




MESHX_END_DECLS

#endif /* _MESHX_CMD_CONMMON_H_ */
