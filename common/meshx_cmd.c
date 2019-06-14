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
#include "meshx_lib.h"

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


static meshx_user_cmd_info_t cmd_info;
static meshx_cmd_parsed_data_t parsed_data;

static const meshx_cmd_t *puser_cmds;
static uint32_t num_user_cmds;

static const char err_data[] = "execute command failed: ";

int32_t meshx_cmd_init(const meshx_cmd_t *pcmds, uint32_t num_cmds)
{
    cmd_info.history_save_index = MESHX_CMD_MAX_HISTORY_SIZE;
    cmd_info.cmd_prompt = '$';
    cmd_info.cmd_crlf[0] = '\r';
    cmd_info.cmd_crlf[1] = '\n';
    meshx_tty_send(&cmd_info.cmd_prompt, 1);
    puser_cmds = pcmds;
    num_user_cmds = num_cmds;
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

    memset(&parsed_data, 0, sizeof(meshx_cmd_parsed_data_t));

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
            parsed_data.pcmd = p;
            return TRUE;
        }
    }
    *q = '\0';
    parsed_data.pcmd = p;
    p = q + 1;

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
                return TRUE;;
            }
        }

        /* find word end */
        q = p;
        while (' ' != *q)
        {
            q ++;
            if (q >= tail)
            {
                *q = '\0';
                /* reach end */
                parsed_data.param_ptr[parsed_data.param_cnt] = p;
                parsed_data.param_val[parsed_data.param_cnt] = meshx_atoi(p, NULL);
                parsed_data.param_cnt ++;
                return TRUE;
            }
        }
        *q = '\0';
        parsed_data.param_ptr[parsed_data.param_cnt] = p;
        parsed_data.param_val[parsed_data.param_cnt] = meshx_atoi(p, NULL);
        parsed_data.param_cnt ++;
        p = q + 1;
    }

    return TRUE;
}

static int32_t meshx_cmd_execute(void)
{
    int32_t ret = -MESHX_ERR_NOT_FOUND;
    for (uint32_t i = 0; i < num_user_cmds; ++i)
    {
        if (0 == strcmp(parsed_data.pcmd, puser_cmds[i].pcmd))
        {
            if (NULL != puser_cmds[i].process)
            {
                ret = puser_cmds[i].process(&parsed_data);
            }
            break;
        }
    }
    return ret;
}

#if 0
static void meshx_send_err_code(int32_t err_code)
{
    if (0 == err_code)
    {
        return ;
    }

    meshx_send_err_code(err_code / 10);
    char data = err_code % 10 + 0x30;
    meshx_tty_send(&data, 1);
}

static void meshx_cmd_send_err_code(int32_t err_code)
{
    if (err_code < 0)
    {
        err_code = -err_code;
        meshx_tty_send("-", 1);
    }

    meshx_send_err_code(err_code);
}
#endif

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
                if (meshx_cmd_parse(cmd_info.cmd, cmd_info.cmd_len))
                {
                    /* execute command */
                    int32_t ret = meshx_cmd_execute();
                    if (MESHX_SUCCESS != ret)
                    {
                        meshx_tty_send(cmd_info.cmd_crlf, 2);
                        meshx_tty_send(err_data, sizeof(err_data));
                        const char *err_str = meshx_errno_to_string(ret);
                        meshx_tty_send(err_str, strlen(err_str));
                    }
                }
#if 0
                MESHX_INFO("parsed cmd: %s", parsed_data.pcmd);
                MESHX_INFO("parsed param cnt: %d", parsed_data.param_cnt);
                for (uint8_t i = 0; i < parsed_data.param_cnt; ++i)
                {
                    MESHX_INFO("parsed param value: %d", parsed_data.param_val[i]);
                }
                for (uint8_t i = 0; i < parsed_data.param_cnt; ++i)
                {
                    MESHX_INFO("parsed param ptr: %s", parsed_data.param_ptr[i]);
                }
#endif


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

