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
MESHX_EXTERN int32_t meshx_cmd_prov_set_public_key(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_prov_public_key(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_prov_set_auth(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_prov_confirmation(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_prov_random(const meshx_cmd_parsed_data_t *pparsed_data);


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
        "prov_cap",\
        "prov_cap [prov id] [element nums] [public key type] [static oob type] [output oob size] [output oob action] [input oob size] [input oob action]",\
        "send provision capabilites pdu.\r\n  element nums: 1-255\r\n  public key type: 0(oob no available) 1(oob available)\r\n  static oob type: 0(not available) 1(available)\r\n  output oob size: 0(not support) 1-8(maximum oob size)\r\n  output oob action: bit0(blink) bit1(beep) bit2(vibrate) bit3(output numeric) bit4(output alpha)\r\n  input oob size: 0(not support) 1-8(maximum oob size)\r\n  input oob action: bit0(push) bit1(twist) bit2(input numeric) bit3(input alpha)",\
        meshx_cmd_prov_capabilites,\
    },\
    {\
        "prov_start",\
        "prov_start [prov_id] [public key] [auth method] [auth action] [auth size]",\
        "send provision start pdu.\r\n  public key: 0(no oob) 1(oob)\r\n  auth method: 0(no oob auth) 1(static oob auth) 2(output oob auth) 3(input oob auth)\r\n  auth action: 0(blink-push) 1(beep-twist) 2(vibrate-input numeric) 3(output numeric-input alpha) 4(output alpha)\r\n  auth size: 1-8",\
        meshx_cmd_prov_start,\
    },\
    {\
        "prov_set_pub_key",\
        "prov_set_pub_key [prov_id] [public key]",\
        "set remote public key, public key is plaintext in hex format",\
        meshx_cmd_prov_set_public_key,\
    },\
    {\
        "prov_pub_key",\
        "prov_pub_key [prov_id]",\
        "send provision public key pdu.",\
        meshx_cmd_prov_public_key,\
    },\
    {\
        "prov_set_auth",\
        "prov_set_auth [prov_id] [mode] [auth action] [auth value]",\
        "set provision auth value.\r\n  mode: 0(no oob) 1(static oob) 2(input/output oob)\r\n  action: 0(blink/beep/vibrate/numeric) 1(alpha), ignore this field if mode is 0 or 1\r\n  auth value: numeric(mode is 2 and action is 0) hex bin(mode is 1) alpha string(mode is 2 and action is 1)",\
        meshx_cmd_prov_set_auth,\
    },\
    {\
        "prov_cfm",\
        "prov_cfm [prov_id]",\
        "send provision confirmation pdu.",\
        meshx_cmd_prov_confirmation,\
    },\
    {\
        "prov_random",\
        "prov_random [prov_id]",\
        "send provision random pdu.",\
        meshx_cmd_prov_random,\
    }





MESHX_END_DECLS

#endif /* _MESHX_CMD_CONMMON_H_ */
