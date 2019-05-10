/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_QUEUE_H_
#define _MESHX_QUEUE_H_

#include "meshx_types.h"

MESHX_BEGIN_DECLS

typedef struct meshx_queue_elem
{
    struct meshx_queue_elem *pnext;
} meshx_queue_elem_t;

typedef struct
{
    meshx_queue_elem_t *pfront;
    meshx_queue_elem_t *pback;
} meshx_queue_t;

void meshx_queue_init(meshx_queue_t *pqueue);
void meshx_queue_push(meshx_queue_t *pqueue, meshx_queue_elem_t *pelem);
meshx_queue_elem_t *meshx_queue_pop(meshx_queue_t *pqueue);

static __INLINE meshx_queue_elem_t *meshx_queue_peek(const meshx_queue_t *pqueue)
{
    return pqueue->pfront;
}

MESHX_END_DECLS


#endif /* _MESHX_QUEUE_H_ */