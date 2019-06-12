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
    uint8_t history_save_index;
    uint8_t history_traverse_index;
    bool history_looped;
    bool history_traversing;
    bool history_traversed_to_head;
    bool history_traversed_to_tail;
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
    pcmd_info->history_save_index = MESHX_CMD_MAX_HISTORY_SIZE;
    pcmd_info->cmd_prompt = '$';
    pcmd_info->cmd_crlf[0] = '\r';
    pcmd_info->cmd_crlf[1] = '\n';
    meshx_tty_send(&pcmd_info->cmd_prompt, 1);
    return MESHX_SUCCESS;
}

static void meshx_cmd_clear(void)
{
    uint8_t cmd_len = 0;
    if (pcmd_info->history_traversing)
    {
        cmd_len = pcmd_info->cmd_history_len[pcmd_info->history_traverse_index];
    }
    else
    {
        cmd_len = pcmd_info->cmd_len;
    }

    for (uint8_t i = 0; i < cmd_len; ++i)
    {
        meshx_tty_send("\b \b", 3);
    }
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
            if ((pcmd_info->cmd_len > 0) || (pcmd_info->history_traversing))
            {
                if (MESHX_CMD_MAX_HISTORY_SIZE == pcmd_info->history_save_index)
                {
                    /* save for first time */
                    pcmd_info->history_save_index = 0;
                }
                else
                {
                    /* adjust save index */
                    pcmd_info->history_save_index ++;
                    if (MESHX_CMD_MAX_HISTORY_SIZE == pcmd_info->history_save_index)
                    {
                        pcmd_info->history_save_index = 0;
                        pcmd_info->history_looped = TRUE;
                    }
                }
                if (pcmd_info->history_traversing)
                {
                    memcpy(pcmd_info->cmd_history[pcmd_info->history_traverse_index],
                           pcmd_info->cmd_history[pcmd_info->history_traverse_index],
                           pcmd_info->cmd_history_len[pcmd_info->history_traverse_index]);
                }
                else
                {
                    memcpy(pcmd_info->cmd_history[pcmd_info->history_save_index], pcmd_info->cmd, pcmd_info->cmd_len);
                }
                pcmd_info->cmd_history_len[pcmd_info->history_save_index] = pcmd_info->cmd_len;
                pcmd_info->history_traverse_index = pcmd_info->history_save_index;
                /* clear cmd */
                pcmd_info->cmd_len = 0;
                pcmd_info->cursor_pos = 0;
                pcmd_info->history_traversed_to_head = FALSE;
                pcmd_info->history_traversed_to_tail = TRUE;
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
            if ((MESHX_CMD_MAX_HISTORY_SIZE != pcmd_info->history_save_index) &&
                !pcmd_info->history_traversed_to_head)
            {
                pcmd_info->history_traversed_to_tail = FALSE;
                pcmd_info->history_traversing = TRUE;
                /* clear current cmd */
                meshx_cmd_clear();
                /* display history */
                meshx_tty_send(pcmd_info->cmd_history[pcmd_info->history_traverse_index],
                               pcmd_info->cmd_history_len[pcmd_info->history_traverse_index]);
                /* adjust traverse index */
                if (pcmd_info->history_looped)
                {
                    if (0 == pcmd_info->history_traverse_index)
                    {
                        pcmd_info->history_traverse_index = MESHX_CMD_MAX_HISTORY_SIZE - 1;
                    }
                    else
                    {
                        pcmd_info->history_traverse_index --;
                    }

                    if (pcmd_info->history_traverse_index == pcmd_info->history_save_index)
                    {
                        pcmd_info->history_traversed_to_head = TRUE;
                    }
                }
                else
                {
                    if (0 == pcmd_info->history_traverse_index)
                    {
                        pcmd_info->history_traversed_to_head = TRUE;
                    }
                    else
                    {
                        pcmd_info->history_traverse_index --;
                    }
                }
            }
            break;
        case '.': /* history next command */
            if ((MESHX_CMD_MAX_HISTORY_SIZE != pcmd_info->history_save_index) &&
                !pcmd_info->history_traversed_to_tail)
            {
                pcmd_info->history_traversed_to_head = FALSE;
                /* clear current cmd */
                meshx_cmd_clear();
                /* adjust traverse index */
                if (pcmd_info->history_looped)
                {
                    pcmd_info->history_traverse_index = (pcmd_info->history_traverse_index + 1) %
                                                        MESHX_CMD_MAX_HISTORY_SIZE;
                    if (pcmd_info->history_traverse_index == pcmd_info->history_save_index)
                    {
                        pcmd_info->history_traversed_to_tail = TRUE;
                    }
                }
                else
                {
                    if (pcmd_info->history_traverse_index == pcmd_info->history_save_index)
                    {
                        pcmd_info->history_traversed_to_tail = TRUE;
                    }
                    else
                    {
                        pcmd_info->history_traverse_index ++;
                    }
                }
                /* display history */
                if (pcmd_info->history_traversed_to_tail)
                {
                    meshx_tty_send(pcmd_info->cmd, pcmd_info->cmd_len);
                    pcmd_info->history_traversing = FALSE;
                }
                else
                {
                    meshx_tty_send(pcmd_info->cmd_history[pcmd_info->history_traverse_index],
                                   pcmd_info->cmd_history_len[pcmd_info->history_traverse_index]);
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

