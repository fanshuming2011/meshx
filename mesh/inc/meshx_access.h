/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_ACCESS_H_
#define _MESHX_ACCESS_H_

#include "meshx_net.h"

MESHX_BEGIN_DECLS

#define MESHX_ACCESS_OPCODE_SIZE(opcode)  ((opcode) >= 0xC00000 ? 3 : ((opcode) >= 0x8000 ? 2 : 1))

MESHX_EXTERN int32_t meshx_access_init(void);
MESHX_EXTERN void meshx_access_opcode_to_buf(uint32_t opcode, uint8_t *pdata);
MESHX_EXTERN uint32_t meshx_access_buf_to_opcode(const uint8_t *pdata);
MESHX_EXTERN int32_t meshx_access_send(const uint8_t *pdata, uint16_t len,
                                       meshx_msg_ctx_t *pmsg_tx_ctx);
MESHX_EXTERN int32_t meshx_access_receive(const uint8_t *pdata,
                                          uint8_t len, meshx_msg_ctx_t *pmsg_rx_ctx);



MESHX_END_DECLS

#endif /* _MESHX_LOWER_TRANS_H_ */