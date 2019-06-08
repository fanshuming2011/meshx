/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#define TRACE_MODULE "MESHX_CMD"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_cmd.h"

typedef struct
{
    char      cmd_line[MESHX_CMD_MAX_CMD_LEN + 2];
    uint8_t   cmd_len;
    uint8_t   cmd_cur;
    uint8_t   cmd_history[MESHX_CMD_MAX_HISTORY_SIZE][MESHX_CMD_MAX_CMD_LEN + 2];
    uint8_t   cmd_history_len[MESHX_CMD_MAX_HISTORY_SIZE];
    uint8_t   history_head;
    uint8_t   history_tail;
    uint8_t   history_cur;
    char      cmd_prompt[2];
    char      cmd_crlf[3];
} user_cmd_if_t;

int32_t meshx_cmd_init(const meshx_cmd_t *pcmds, uint32_t num_cmds)
{
    return MESHX_SUCCESS;
}

void meshx_cmd_parse(const uint8_t *pdata, uint8_t len)
{
    for (uint8_t i = 0; i < len; ++i)
    {
        switch (pdata[i])
        {
        case '\r':
        case '\n':
            break;
        case 0x08: /* backspace */
            break;
        default:
            break;
        }
    }
}

void meshx_cmd_list(const char *pcmd)
{

}

void meshx_cmd_list_all(void)
{

}

