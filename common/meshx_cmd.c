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
    char cmd_temp[MESHX_CMD_MAX_LEN + 2];
    uint8_t cmd_temp_len;
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

#define MESHX_CMD_MAX_PARAMETERS              20
typedef struct
{
    char *pcmd;
    uint8_t param_cnt;
    uint32_t param_val[MESHX_CMD_MAX_PARAMETERS];
    char *param_ptr[MESHX_CMD_MAX_PARAMETERS];
} meshx_parsed_cmd_t;

static meshx_user_cmd_info_t cmd_info;
static meshx_parsed_cmd_t parsed_cmd;

int32_t meshx_cmd_init(const meshx_cmd_t *pcmds, uint32_t num_cmds)
{
    cmd_info.history_save_index = MESHX_CMD_MAX_HISTORY_SIZE;
    cmd_info.cmd_prompt = '$';
    cmd_info.cmd_crlf[0] = '\r';
    cmd_info.cmd_crlf[1] = '\n';
    meshx_tty_send(&cmd_info.cmd_prompt, 1);
    return MESHX_SUCCESS;
}

static void meshx_cmd_clear(void)
{
    for (uint8_t i = 0; i < cmd_info.cmd_len; ++i)
    {
        meshx_tty_send("\b \b", 3);
    }
}

static void meshx_cmd_move_forward(void)
{
    for (uint8_t i = 0; i <= cmd_info.cmd_len - cmd_info.cursor_pos; ++i)
    {
        cmd_info.cmd[cmd_info.cursor_pos + i - 1] = cmd_info.cmd[cmd_info.cursor_pos + i];
    }
    cmd_info.cmd[cmd_info.cmd_len - 1] = ' ';
    meshx_tty_send("\b", 1);
}

static void meshx_cmd_move_back(void)
{
    for (uint8_t i = 0; i < cmd_info.cmd_len - cmd_info.cursor_pos; ++i)
    {
        cmd_info.cmd[cmd_info.cmd_len - i] = cmd_info.cmd[cmd_info.cmd_len - i - 1];
    }
}

static void meshx_cmd_cursor_move_back(void)
{
    for (uint8_t i = 0; i < cmd_info.cmd_len - cmd_info.cursor_pos; ++i)
    {
        meshx_tty_send("\b", 1);
    }
}

static bool meshx_cmd_parse(char *pdata, uint8_t len)
{
    MESHX_INFO("parse cmd: %d-%s", len, pdata);
    const char *tail = pdata + len;
    char *p = pdata;
    char *q;

    memset(&parsed_cmd, 0, sizeof(meshx_parsed_cmd_t));

    /* parse command */
    /* skip space */
    while (' ' == *p)
    {
        p ++;
        if (p >= tail)
        {
            /* no valid command */
            return FALSE;
        }
    };

    /* find word end */
    q = p;
    while (' ' != *q)
    {
        q ++;
        if (q >= tail)
        {
            *q = '\0';
            /* reach end */
            parsed_cmd.pcmd = p;
            return TRUE;
        }
    };
    *q = '\0';
    parsed_cmd.pcmd = p;

    /* parse parameters */
    while (1)
    {
        /* skip space */
        while (' ' == *p)
        {
            p ++;
            if (p >= tail)
            {
                /* parse finished */
                break;
            }
        };

        /* find word end */
        q = p;
        while (' ' != *q)
        {
            q ++;
            if (q >= tail)
            {
                *q = '\0';
                /* reach end */
                parsed_cmd.param_ptr[parsed_cmd.param_cnt] = p;
                //parsed_cmd.param_val = *p;
                break;
            }
        };
        *q = '\0';
        parsed_cmd.param_ptr[parsed_cmd.param_cnt] = p;
        //parsed_cmd.param_val = *p;
    }

    return TRUE;
}

void meshx_cmd_collect(const uint8_t *pdata, uint8_t len)
{
    for (uint8_t i = 0; i < len; ++i)
    {
        switch (pdata[i])
        {
        case '\r':
        case '\n': /* command input finished */
            if (cmd_info.cmd_len > 0)
            {
                if (MESHX_CMD_MAX_HISTORY_SIZE == cmd_info.history_save_index)
                {
                    /* save for first time */
                    cmd_info.history_save_index = 0;
                }
                else
                {
                    /* adjust save index */
                    cmd_info.history_save_index ++;
                    if (MESHX_CMD_MAX_HISTORY_SIZE == cmd_info.history_save_index)
                    {
                        cmd_info.history_save_index = 0;
                        cmd_info.history_looped = TRUE;
                    }
                }

                memcpy(cmd_info.cmd_history[cmd_info.history_save_index], cmd_info.cmd, cmd_info.cmd_len);
                cmd_info.cmd_history_len[cmd_info.history_save_index] = cmd_info.cmd_len;
                cmd_info.cmd_history[cmd_info.history_save_index][cmd_info.cmd_len] = '\0';
                cmd_info.cmd[cmd_info.cmd_len] = '\0';

                /* parse command */
                meshx_cmd_parse(cmd_info.cmd, cmd_info.cmd_len);
                MESHX_INFO("parsed cmd: %s", parsed_cmd.pcmd);
                MESHX_INFO("parsed param cnt: %d", parsed_cmd.param_cnt);
                for (uint8_t i = 0; i < parsed_cmd.param_cnt; ++i)
                {
                    MESHX_INFO("parsed param value: %d", parsed_cmd.param_val);
                }
                for (uint8_t i = 0; i < parsed_cmd.param_cnt; ++i)
                {
                    MESHX_INFO("parsed param ptr: %s", parsed_cmd.param_ptr);
                }

                /* execute command */

                /* clear cmd */
                cmd_info.cmd_len = 0;
                cmd_info.cursor_pos = 0;
                cmd_info.history_traverse_index = cmd_info.history_save_index;
                cmd_info.history_traversed_to_head = FALSE;
                cmd_info.history_traversed_to_tail = TRUE;
                cmd_info.history_traversing = FALSE;
            }

            /* output new line */
            meshx_tty_send(cmd_info.cmd_crlf, 2);
            meshx_tty_send(&cmd_info.cmd_prompt, 1);
            break;
        case 127: /* backspace */
        case '\b': /* backspace */
            if ((cmd_info.cursor_pos > 0) && (cmd_info.cmd_len > 0))
            {
                meshx_cmd_move_forward();
                cmd_info.cursor_pos --;
                meshx_tty_send(cmd_info.cmd + cmd_info.cursor_pos, cmd_info.cmd_len - cmd_info.cursor_pos);
                cmd_info.cmd[cmd_info.cmd_len - 1] = '\0';
                meshx_cmd_cursor_move_back();
                cmd_info.cmd_len --;
            }
            break;
        case '[': /* cursor move left */
            if (cmd_info.cursor_pos > 0)
            {
                meshx_tty_send("\b", 1);
                cmd_info.cursor_pos --;
            }
            break;
        case ']': /* cursor move right */
            if (cmd_info.cursor_pos < cmd_info.cmd_len)
            {
                meshx_tty_send(&cmd_info.cmd[cmd_info.cursor_pos], 1);
                cmd_info.cursor_pos ++;
            }
            break;
        case ',': /* history previous command */
            if ((MESHX_CMD_MAX_HISTORY_SIZE != cmd_info.history_save_index) &&
                !cmd_info.history_traversed_to_head)
            {
                cmd_info.history_traversed_to_tail = FALSE;
                if (cmd_info.history_traversing)
                {
                    /* adjust traverse index */
                    if (cmd_info.history_looped)
                    {
                        if (cmd_info.history_traverse_index == ((cmd_info.history_save_index + 1) %
                                                                MESHX_CMD_MAX_HISTORY_SIZE))
                        {
                            cmd_info.history_traversed_to_head = TRUE;
                        }
                        else
                        {
                            if (0 == cmd_info.history_traverse_index)
                            {
                                cmd_info.history_traverse_index = MESHX_CMD_MAX_HISTORY_SIZE - 1;
                            }
                            else
                            {
                                cmd_info.history_traverse_index --;
                            }
                        }
                    }
                    else
                    {
                        if (0 == cmd_info.history_traverse_index)
                        {
                            cmd_info.history_traversed_to_head = TRUE;
                        }
                        else
                        {
                            cmd_info.history_traverse_index --;
                        }
                    }
                }
                else
                {
                    cmd_info.history_traversing = TRUE;
                    memcpy(cmd_info.cmd_temp, cmd_info.cmd, cmd_info.cmd_len);
                    cmd_info.cmd_temp_len = cmd_info.cmd_len;
                }

                if (!cmd_info.history_traversed_to_head)
                {
                    /* clear command */
                    meshx_cmd_clear();
                    /* copy history */
                    memcpy(cmd_info.cmd, cmd_info.cmd_history[cmd_info.history_traverse_index],
                           cmd_info.cmd_history_len[cmd_info.history_traverse_index]);
                    cmd_info.cmd_len = cmd_info.cmd_history_len[cmd_info.history_traverse_index];
                    cmd_info.cursor_pos = cmd_info.cmd_len;

                    /* display history */
                    meshx_tty_send(cmd_info.cmd, cmd_info.cmd_len);
                }
            }
            break;
        case '.': /* history next command */
            if ((MESHX_CMD_MAX_HISTORY_SIZE != cmd_info.history_save_index) &&
                !cmd_info.history_traversed_to_tail)
            {
                cmd_info.history_traversed_to_head = FALSE;
                /* clear current cmd */
                meshx_cmd_clear();
                /* adjust traverse index */
                if (cmd_info.history_looped)
                {
                    if (cmd_info.history_traverse_index == cmd_info.history_save_index)
                    {
                        cmd_info.history_traversed_to_tail = TRUE;
                    }
                    else
                    {
                        cmd_info.history_traverse_index = (cmd_info.history_traverse_index + 1) %
                                                          MESHX_CMD_MAX_HISTORY_SIZE;
                    }
                }
                else
                {
                    if (cmd_info.history_traverse_index == cmd_info.history_save_index)
                    {
                        cmd_info.history_traversed_to_tail = TRUE;
                    }
                    else
                    {
                        cmd_info.history_traverse_index ++;
                    }
                }
                /* display history */
                if (cmd_info.history_traversed_to_tail)
                {
                    meshx_tty_send(cmd_info.cmd_temp, cmd_info.cmd_temp_len);
                    memcpy(cmd_info.cmd, cmd_info.cmd_temp, cmd_info.cmd_temp_len);
                    cmd_info.cmd_len = cmd_info.cmd_temp_len;
                    cmd_info.cursor_pos = cmd_info.cmd_len;
                    cmd_info.history_traversing = FALSE;
                }
                else
                {
                    meshx_tty_send(cmd_info.cmd_history[cmd_info.history_traverse_index],
                                   cmd_info.cmd_history_len[cmd_info.history_traverse_index]);
                    memcpy(cmd_info.cmd, cmd_info.cmd_history[cmd_info.history_traverse_index],
                           cmd_info.cmd_history_len[cmd_info.history_traverse_index]);
                    cmd_info.cmd_len = cmd_info.cmd_history_len[cmd_info.history_traverse_index];
                    cmd_info.cursor_pos = cmd_info.cmd_len;
                }
            }
            break;
        default:
            if (cmd_info.cmd_len < MESHX_CMD_MAX_LEN)
            {
                meshx_cmd_move_back();
                cmd_info.cmd[cmd_info.cursor_pos] = pdata[i];
                cmd_info.cmd_len ++;
                cmd_info.cursor_pos ++;
                meshx_tty_send(cmd_info.cmd + cmd_info.cursor_pos - 1,
                               cmd_info.cmd_len - cmd_info.cursor_pos + 1);
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

