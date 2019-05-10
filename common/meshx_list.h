/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_LIST_H_
#define _MESHX_LIST_H_

#include "meshx_types.h"
#include "meshx_assert.h"

MESHX_BEGIN_DECLS


typedef struct meshx_list
{
    struct meshx_list *pprev;
    struct meshx_list *pnext;
} meshx_list_t;

/* return true if want to stop */
typedef bool (*meshx_list_node_cmp_t)(const meshx_list_t *pnode_insert,
                                      const meshx_list_t *pnode_list);

//extern void meshx_list_init_head(meshx_list_t *phead);
static __INLINE void meshx_list_init_head(meshx_list_t *phead)
{
    MESHX_ASSERT(NULL != phead);
    phead->pprev = phead;
    phead->pnext = phead;

}

static __INLINE void meshx_list_init_node(meshx_list_t *pnode)
{
    MESHX_ASSERT(NULL != pnode);
    pnode->pprev = NULL;
    pnode->pnext = NULL;
}

MESHX_EXTERN void meshx_list_prepend(meshx_list_t *phead, meshx_list_t *pnode);
MESHX_EXTERN void meshx_list_append(meshx_list_t *phead, meshx_list_t *pnode);
MESHX_EXTERN void meshx_list_sort_insert(meshx_list_t *phead, meshx_list_t *pnode,
                                         meshx_list_node_cmp_t node_cmp);
MESHX_EXTERN meshx_list_t *meshx_list_pop(meshx_list_t *phead);
MESHX_EXTERN void meshx_list_remove(meshx_list_t *pnode);
MESHX_EXTERN uint32_t meshx_list_index(const meshx_list_t *phead, const meshx_list_t *pnode);
MESHX_EXTERN bool meshx_list_exists(const meshx_list_t *phead, const meshx_list_t *pnode);
MESHX_EXTERN meshx_list_t *mesh_list_peek(const meshx_list_t *phead);

static __INLINE bool meshx_list_is_empty(const meshx_list_t *phead)
{
    MESHX_ASSERT(NULL != phead);

    return (phead->pnext == phead);
}

static __INLINE bool meshx_list_is_last(const meshx_list_t *phead, const meshx_list_t *pnode)
{
    MESHX_ASSERT(NULL != phead);

    return (phead->pprev == pnode);
}

static __INLINE bool meshx_list_is_first(const meshx_list_t *phead, const meshx_list_t *pnode)
{
    MESHX_ASSERT(NULL != phead);

    return (phead->pnext == pnode);
}

MESHX_EXTERN uint32_t meshx_list_length(const meshx_list_t *phead);

/**
 * @brief list
 * @param pos - current node pointer
 * @param head - list head
 */
#define meshx_list_foreach(ppos, phead) \
    for (ppos = (phead)->pnext; ppos != (phead); ppos = ppos->pnext)


MESHX_END_DECLS




#endif