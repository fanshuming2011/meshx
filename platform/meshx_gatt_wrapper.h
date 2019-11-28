/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_GATT_WRAPPER_H_
#define _MESHX_GATT_WRAPPER_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef struct
{
    bool is_uuid16;
    union
    {
        uint16_t uuid16;
        uint8_t uuid128[16];
    };
} meshx_gatts_uuid_t;

typedef enum
{
    MESHX_PRIMARY_SERVICE = 1,
    MESHX_SECONDARY_SERVICE,
} meshx_gatts_service_t;

typedef struct
{
    uint16_t reliable_write : 1;
    uint16_t writeable : 1;
} meshx_gatts_char_desc_ext_prop_t;

typedef struct
{
    char *string;
    uint8_t len;
} meshx_gatts_char_desc_user_desc_t;

typedef struct
{
    uint8_t format;
    uint8_t exponent;
    uint16_t unit;
    uint8_t name_space;
    uint16_t desc;
} meshx_gatts_char_desc_cpf_t;

typedef struct
{
    meshx_gatts_char_desc_ext_prop_t *pextend_prop;
    meshx_gatts_char_desc_cpf_t
    *pchar_format;     // See more details at Bluetooth SPEC 4.2 [Vol 3, Part G] Page 539
    meshx_gatts_char_desc_user_desc_t *puser_desc;       // read only
} meshx_gatts_char_desc_db_t;

typedef enum
{
    MESHX_CHAR_PROPERTY_BROADCAST = 1,
    MESHX_CHAR_PROPERTY_READ,
    MESHX_CHAR_PROPERTY_WRITE_WITHOUT_RESP,
    MESHX_CHAR_PROPERTY_WRITE,
    MESHX_CHAR_PROPERTY_NOTIFY,
    MESHX_CHAR_PROPERTY_INDICATE,
    MESHX_CHAR_PROPERTY_AUTH_SIGNED_WRITE,
} meshx_gatts_char_property;

typedef struct
{
    meshx_gatts_uuid_t char_uuid;
    meshx_gatts_char_property char_property;
    uint8_t *pvalue;
    uint8_t char_value_len; /* pvalue length or pvalue maximum length when is_variable_len is TRUE */
    bool is_variable_len;
    bool read_auth;
    bool writer_auth;
    meshx_gatts_char_desc_db_t char_desc_db;

    uint16_t char_value_handle; /*[out] store assigned characteristic handle */
} meshx_gatts_char_db_t;

typedef struct
{
    meshx_gatts_service_t srv_type;
    meshx_gatts_uuid_t srv_uuid;
    uint8_t char_num;
    meshx_gatts_char_db_t *pchar_db;

    uint16_t srv_handle; /* [out] store assigned service handle */
} meshx_gatts_srv_db_t;

typedef struct
{
    uint8_t srv_num;
    meshx_gatts_srv_db_t *psrv_db;
} meshx_gatts_db_t;

int32_t meshx_gatts_service_init(meshx_gatts_db_t *pservice_db);
int32_t meshx_gatts_notify(uint16_t conn_handle, uint16_t char_value_handle,
                           const uint8_t *pdata, uint16_t len);


MESHX_END_DECLS

#endif /* _MESHX_GATT_WRAPPER_H_ */
