/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_queue.h"
#include "meshx_assert.h"

void meshx_queue_init(meshx_queue_t *pqueue)
{
    MESHX_ASSERT(NULL != pqueue);
    pqueue->pfront = NULL;
    pqueue->pback = NULL;
}

void meshx_queue_push(meshx_queue_t *pqueue, meshx_queue_elem_t *pelem)
{
    MESHX_ASSERT(NULL != pqueue);
    MESHX_ASSERT(NULL != pelem);
    pelem->pnext = NULL;
    if (NULL == pqueue->pfront)
    {
        pqueue->pfront = pelem;
    }
    else
    {
        pqueue->pback->pnext = pelem;
    }

    pqueue->pback = pelem;
}

meshx_queue_elem_t *meshx_queue_pop(meshx_queue_t *pqueue)
{
    MESHX_ASSERT(NULL != pqueue);

    meshx_queue_elem_t *pelem = pqueue->pfront;
    if (NULL != pelem)
    {
        pqueue->pfront = pelem->pnext;
    }

    if (NULL == pqueue->pfront)
    {
        pqueue->pback = NULL;
    }

    return pelem;
}
