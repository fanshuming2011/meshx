/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_H_
#define _MESHX_H_

#include "meshx_types.h"
#include "meshx_errno.h"
#include "meshx_io.h"
#include "meshx_lib.h"
#include "meshx_trace.h"
#include "meshx_async.h"
#include "meshx_tty.h"
#include "meshx_cmd.h"
#include "meshx_gap.h"
#include "meshx_bearer.h"
#include "meshx_network.h"
#include "meshx_provision.h"
#include "meshx_node.h"
#include "meshx_beacon.h"
#include "meshx_notify.h"
#include "meshx_common.h"
#include "meshx_mem.h"
#include "meshx_misc.h"
#include "meshx_sample_data.h"
#include "meshx_rpl.h"
#include "meshx_nmc.h"
#include "meshx_seq.h"
#include "meshx_iv_index.h"
#include "meshx_key.h"
#include "meshx_lower_transport.h"


MESHX_BEGIN_DECLS

MESHX_EXTERN int32_t meshx_init(void);
MESHX_EXTERN int32_t meshx_run(void);

MESHX_END_DECLS


#endif /* _MESHX_H_ */

