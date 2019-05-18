/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_PROVISION_H_
#define _MESHX_PROVISION_H_

#include "meshx_bearer.h"

MESHX_BEGIN_DECLS

typedef struct
{
    uint8_t attention_duration;
} __PACKED meshx_provision_invite_t;

typedef struct
{
    uint8_t element_nums;
    uint16_t algorithms;
    uint8_t public_key_type;
    uint8_t static_oob_type;
    uint8_t output_oob_type;
    uint16_t output_oob_action;
    uint8_t input_oob_size;
    uint8_t input_oob_action;
} meshx_provision_capabilites_t;

typedef bool (*meshx_provision_callback_t)(uint8_t state, void *pargs);

MESHX_EXTERN int32_t meshx_provision_init(void);

MESHX_EXTERN int32_t meshx_provision_link_open(meshx_bearer_t bearer, meshx_dev_uuid_t dev_uuid);
MESHX_EXTERN int32_t meshx_provision_link_ack(meshx_bearer_t bearer, uint32_t link_id);
MESHX_EXTERN int32_t meshx_provision_link_close(meshx_bearer_t bearer, uint32_t link_id,
                                                uint8_t reason);

MESHX_EXTERN int32_t meshx_provision_invite(meshx_bearer_t bearer, uint32_t link_id,
                                            meshx_provision_invite_t invite);

MESHX_END_DECLS


#endif /* _MESHX_PROVISION_H_ */
