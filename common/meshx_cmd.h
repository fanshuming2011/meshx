/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_CMD_H_
#define _MESHX_CMD_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

#define MESHX_CMD_MAX_CMD_LEN                 70
#define MESHX_CMD_MAX_PARAMETERS              20
#define MESHX_CMD_MAX_HISTORY_SIZE            3

typedef struct
{
    char *pcmd;
    uint8_t param_count;
    uint32_t parameters[MESHX_CMD_MAX_PARAMETERS];
} meshx_cmd_parsed_data_t;

typedef void (*meshx_tty_send_t)(const uint8_t *pdata, uint32_t len);
typedef int32_t (*meshx_cmd_process_t)(const meshx_cmd_parsed_data_t *pdata);

typedef struct
{
    char *pcmd;
    char *pusage;
    char *phelp;
    meshx_cmd_process_t *pprocess;
} meshx_cmd_t;

MESHX_EXTERN int32_t meshx_cmd_init(const meshx_cmd_t *pcmds, uint32_t num_cmds);
MESHX_EXTERN void meshx_cmd_parse(const uint8_t *pdata, uint8_t len);
MESHX_EXTERN void meshx_cmd_list(const char *pcmd);
MESHX_EXTERN void meshx_cmd_list_all(void);

MESHX_END_DECLS

#endif /* _MESHX_CMD_H_ */
