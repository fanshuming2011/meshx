/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_CMD_BASE_H_
#define _MESHX_CMD_BASE_H_

#include "meshx_cmd.h"

MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_cmd_help(const meshx_cmd_parsed_data_t *pparsed_data);
MESHX_EXTERN int32_t meshx_cmd_reboot(const meshx_cmd_parsed_data_t *pparsed_data);


#define MESHX_CMD_BASE \
    {\
        "help",\
        "help [cmd]",\
        "list command usage and help, use command name as a parameter or '*' to list all commands",\
        meshx_cmd_help\
    },\
    {\
        "reboot",\
        "reboot",\
        "reboot system",\
        meshx_cmd_reboot \
    }


MESHX_END_DECLS

#endif /* _MESHX_CMD_CONMMON_H_ */
