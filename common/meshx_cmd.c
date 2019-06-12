/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include <stdio.h>
#include <string.h>
#define TRACE_MODULE "MESHX_CMD"
#include "meshx_trace.h"
#include "meshx_errno.h"
#include "meshx_cmd.h"
#include "meshx_tty.h"
#include "meshx_mem.h"
#include "meshx_assert.h"

typedef struct
{
    char cmd[MESHX_CMD_MAX_LEN + 2];
    uint8_t cmd_len;
    uint8_t cursor_pos;
    char cmd_history[MESHX_CMD_MAX_HISTORY_SIZE][MESHX_CMD_MAX_LEN + 2];
    uint8_t cmd_history_len[MESHX_CMD_MAX_HISTORY_SIZE];
    uint8_t history_head_index;
    uint8_t history_tail_index;
    uint8_t history_cur_index;
    char cmd_prompt;
    char cmd_crlf[2];
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
    memset(pcmd_info, 0, sizeof(meshx_user_cmd_info_t));
    pcmd_info->history_tail_index = MESHX_CMD_MAX_HISTORY_SIZE;
    pcmd_info->history_cur_index = MESHX_CMD_MAX_HISTORY_SIZE;
    pcmd_info->cmd_prompt = '$';
    pcmd_info->cmd_crlf[0] = '\r';
    pcmd_info->cmd_crlf[1] = '\n';
    meshx_tty_send(&pcmd_info->cmd_prompt, 1);
    return MESHX_SUCCESS;
}

static void meshx_cmd_clear(void)
{
    for (uint8_t i = 0; i < pcmd_info->cmd_len; ++i)
    {
        pcmd_info->cmd[i] = ' ';
        meshx_tty_send("\b", 1);
    }
    meshx_tty_send(pcmd_info->cmd, pcmd_info->cmd_len);
    for (uint8_t i = 0; i < pcmd_info->cmd_len; ++i)
    {
        meshx_tty_send("\b", 1);
    }
    pcmd_info->cmd_len = 0;
}

static void meshx_cmd_move_forward(void)
{
    for (uint8_t i = 0; i <= pcmd_info->cmd_len - pcmd_info->cursor_pos; ++i)
    {
        pcmd_info->cmd[pcmd_info->cursor_pos + i - 1] = pcmd_info->cmd[pcmd_info->cursor_pos + i];
    }
    /* '\0' does not output anything on linux, so use ' ' instead */
    pcmd_info->cmd[pcmd_info->cmd_len - 1] = ' ';
    meshx_tty_send("\b", 1);
}

static void meshx_cmd_move_back(void)
{
    for (uint8_t i = 0; i < pcmd_info->cmd_len - pcmd_info->cursor_pos; ++i)
    {
        pcmd_info->cmd[pcmd_info->cmd_len - i] = pcmd_info->cmd[pcmd_info->cmd_len - i - 1];
    }
}

static void meshx_cmd_cursor_move_back(void)
{
    for (uint8_t i = 0; i < pcmd_info->cmd_len - pcmd_info->cursor_pos; ++i)
    {
        meshx_tty_send("\b", 1);
    }
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
            if (pcmd_info->cmd_len > 0)
            {
                if (MESHX_CMD_MAX_HISTORY_SIZE == pcmd_info->history_tail_index)
                {
                    /* initialize tail for the first time */
                    pcmd_info->history_tail_index = 0;
                }
                else
                {
                    /* adjust head and tail */
                    pcmd_info->history_tail_index = (pcmd_info->history_tail_index + 1) % MESHX_CMD_MAX_HISTORY_SIZE;

                    if (pcmd_info->history_tail_index == pcmd_info->history_head_index)
                    {
                        pcmd_info->history_head_index = (pcmd_info->history_head_index + 1) % MESHX_CMD_MAX_HISTORY_SIZE;
                    }
                    pcmd_info->history_cur_index = pcmd_info->history_tail_index;
                }
                /* save command */
                memcpy(pcmd_info->cmd_history[pcmd_info->history_tail_index], pcmd_info->cmd, pcmd_info->cmd_len);
                pcmd_info->cmd_history_len[pcmd_info->history_tail_index] = pcmd_info->cmd_len;
                /* clear cmd */
                pcmd_info->cmd_len = 0;
                pcmd_info->cursor_pos = 0;
            }

            meshx_tty_send(pcmd_info->cmd_crlf, 2);
            meshx_tty_send(&pcmd_info->cmd_prompt, 1);
            break;
        case 127: /* backspace */
        case '\b': /* backspace */
            if ((pcmd_info->cursor_pos > 0) && (pcmd_info->cmd_len > 0))
            {
                meshx_cmd_move_forward();
                pcmd_info->cursor_pos --;
                meshx_tty_send(pcmd_info->cmd + pcmd_info->cursor_pos, pcmd_info->cmd_len - pcmd_info->cursor_pos);
                pcmd_info->cmd[pcmd_info->cmd_len - 1] = '\0';
                meshx_cmd_cursor_move_back();
                pcmd_info->cmd_len --;
            }
            break;
        case '[': /* cursor move left */
            if (pcmd_info->cursor_pos > 0)
            {
                meshx_tty_send("\b", 1);
                pcmd_info->cursor_pos --;
            }
            break;
        case ']': /* cursor move right */
            if (pcmd_info->cursor_pos < pcmd_info->cmd_len)
            {
                meshx_tty_send(&pcmd_info->cmd[pcmd_info->cursor_pos], 1);
                pcmd_info->cursor_pos ++;
            }
            break;
        case ',': /* history previous command */
            if ((MESHX_CMD_MAX_HISTORY_SIZE != pcmd_info->history_tail_index) &&
                (pcmd_info->history_head_index != pcmd_info->history_cur_index))
            {
                /* clear current cmd */
                meshx_cmd_clear();
                if (MESHX_CMD_MAX_HISTORY_SIZE == pcmd_info->history_cur_index)
                {
                    /* initialize current for the first time */
                    pcmd_info->history_cur_index = 0;
                }

                /* display history */
                memcpy(pcmd_info->cmd, pcmd_info->cmd_history[pcmd_info->history_cur_index],
                       pcmd_info->cmd_history_len[pcmd_info->history_cur_index]);
                pcmd_info->cmd_len = pcmd_info->cmd_history_len[pcmd_info->history_cur_index];
                pcmd_info->cursor_pos = pcmd_info->cmd_len;
                meshx_tty_send(pcmd_info->cmd, pcmd_info->cmd_len);
                /* adjust current index */
                if (pcmd_info->history_cur_index != pcmd_info->history_head_index)
                {
                    if (0 == pcmd_info->history_cur_index)
                    {
                        pcmd_info->history_cur_index = MESHX_CMD_MAX_HISTORY_SIZE - 1;
                    }
                    else
                    {
                        pcmd_info->history_cur_index --;
                    }
                }
            }
            break;
        case '.': /* history next command */
            if ((MESHX_CMD_MAX_HISTORY_SIZE != pcmd_info->history_tail_index) &&
                (pcmd_info->history_tail_index != pcmd_info->history_cur_index))
            {
                /* clear current cmd */
                meshx_cmd_clear();
                if (MESHX_CMD_MAX_HISTORY_SIZE == pcmd_info->history_cur_index)
                {
                    break;
                }

                /* display history */
                memcpy(pcmd_info->cmd, pcmd_info->cmd_history[pcmd_info->history_cur_index],
                       pcmd_info->cmd_history_len[pcmd_info->history_cur_index]);
                pcmd_info->cmd_len = pcmd_info->cmd_history_len[pcmd_info->history_cur_index];
                pcmd_info->cursor_pos = pcmd_info->cmd_len;
                meshx_tty_send(pcmd_info->cmd, pcmd_info->cmd_len);
                /* adjust current index */
                if (pcmd_info->history_cur_index != pcmd_info->history_tail_index)
                {
                    pcmd_info->history_cur_index = (pcmd_info->history_cur_index + 1) % MESHX_CMD_MAX_HISTORY_SIZE;
                }
            }
            break;
        default:
            if (pcmd_info->cmd_len < MESHX_CMD_MAX_LEN)
            {
                meshx_cmd_move_back();
                pcmd_info->cmd[pcmd_info->cursor_pos] = pdata[i];
                pcmd_info->cmd_len ++;
                pcmd_info->cursor_pos ++;
                meshx_tty_send(pcmd_info->cmd + pcmd_info->cursor_pos - 1,
                               pcmd_info->cmd_len - pcmd_info->cursor_pos + 1);
                meshx_cmd_cursor_move_back();
            }
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

