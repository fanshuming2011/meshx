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
#include "meshx_tty.h"
#include "meshx_mem.h"
#include "meshx_assert.h"

typedef struct
{
    char cmd[MESHX_CMD_MAX_CMD_LEN + 2];
    uint8_t cmd_len;
    uint8_t cursor_pos;
    uint8_t cmd_history[MESHX_CMD_MAX_HISTORY_SIZE][MESHX_CMD_MAX_CMD_LEN + 2];
    uint8_t cmd_history_len[MESHX_CMD_MAX_HISTORY_SIZE];
    uint8_t history_head;
    uint8_t history_tail;
    uint8_t history_cur;
    char cmd_prompt[2];
    char cmd_crlf[3];
} meshx_user_cmd_info_t;

static meshx_user_cmd_info_t *pcmd_info;

int32_t meshx_cmd_init(const meshx_cmd_t *pcmds, uint32_t num_cmds)
{
    pcmd_info = meshx_malloc(sizeof(meshx_user_cmd_info_t));
    if (NULL == pcmd_info)
    {
        MESHX_ERROR("initialize user command failed: out of memory!");
        return MESHX_ERR_MEM;
    }
    pcmd_info->cmd_prompt[0] = '$';
    pcmd_info->cmd_prompt[1] = 0;
    pcmd_info->cmd_crlf[0] = '\n';
    pcmd_info->cmd_crlf[1] = 0;
    return MESHX_SUCCESS;
}

void meshx_cmd_parse(const uint8_t *pdata, uint8_t len)
{
    MESHX_ASSERT(NULL != pcmd_info);
    for (uint8_t i = 0; i < len; ++i)
    {
        switch (pdata[i])
        {
        case '\r':
        case '\n': /* command input finished */
            break;
        case '\b': /* backspace */
            break;
        case '[': /* cursor move left */
            meshx_tty_send("\b", 1);
            break;
        case ']': /* cursor move right */
            break;
        case ',': /* history previous command */
            break;
        case '.': /* history next command */
            break;
        default:
            pcmd_info->cmd[pcmd_info->cursor_pos] = pdata[i];
            meshx_tty_send((const char *)pdata, 1);
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

