/**
 * This file is part of the meshx Library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#include "meshx_list.h"
#include "meshx_assert.h"
/*
void meshx_list_init_head(meshx_list_t *phead)
{
    MESHX_ASSERT(NULL != phead);
    phead->pprev = phead;
    phead->pnext = phead;

}
*/
/**
 * @brief prepend node to list
 * @param head - list head
 * @param node - node to prepend
 */
void meshx_list_prepend(meshx_list_t *phead, meshx_list_t *pnode)
{
    MESHX_ASSERT(NULL != phead);
    MESHX_ASSERT(NULL != pnode);

    phead->pnext->pprev = pnode;
    pnode->pnext = phead->pnext;
    phead->pnext = pnode;
    pnode->pprev = phead;
}

/**
 * @brief append node to list
 * @param head - list head
 * @param node - node to append
 */
void meshx_list_append(meshx_list_t *phead, meshx_list_t *pnode)
{
    MESHX_ASSERT(NULL != phead);
    MESHX_ASSERT(NULL != pnode);

    phead->pprev->pnext = pnode;
    pnode->pprev = phead->pprev;
    phead->pprev = pnode;
    pnode->pnext = phead;
}

void meshx_list_sort_insert(meshx_list_t *phead, meshx_list_t *pnode,
                            meshx_list_node_cmp_t node_cmp)
{
    meshx_list_t *ptemp_node;
    for (ptemp_node = phead->pnext; ptemp_node != phead; ptemp_node = ptemp_node->pnext)
    {
        if (node_cmp(pnode, ptemp_node))
        {
            break;
        }
    }

    ptemp_node->pprev->pnext = pnode;
    pnode->pprev = ptemp_node->pprev;
    ptemp_node->pprev = pnode;
    pnode->pnext = ptemp_node;
}

meshx_list_t *meshx_list_pop(meshx_list_t *phead)
{
    MESHX_ASSERT(NULL != phead);
    if (phead->pnext == phead)
    {
        return NULL;
    }

    meshx_list_t *pnode = phead->pnext;
    pnode->pprev->pnext = pnode->pnext;
    pnode->pnext->pprev = pnode->pprev;
    pnode->pnext = NULL;
    pnode->pprev = NULL;
    return pnode;
}

/**
 * @brief remove node from list
 * @param node - node to remove
 */
void meshx_list_remove(meshx_list_t *pnode)
{
    MESHX_ASSERT(NULL != pnode->pprev);
    MESHX_ASSERT(NULL != pnode->pnext);

    pnode->pprev->pnext = pnode->pnext;
    pnode->pnext->pprev = pnode->pprev;
    pnode->pnext = NULL;
    pnode->pprev = NULL;
}


uint32_t meshx_list_index(const meshx_list_t *phead, const meshx_list_t *pnode)
{
    MESHX_ASSERT(NULL != phead);
    MESHX_ASSERT(NULL != pnode);

    uint32_t index = 0;
    meshx_list_t *ptemp = phead->pnext;
    for (; ptemp != phead; ptemp = ptemp->pnext)
    {
        if (ptemp == pnode)
        {
            return index;
        }
        ++ index;
    }

    return -1;

}

bool meshx_list_exists(const meshx_list_t *phead, const meshx_list_t *pnode)
{
    MESHX_ASSERT(NULL != phead);
    MESHX_ASSERT(NULL != pnode);

    meshx_list_t *ptemp = phead->pnext;
    for (; ptemp != phead; ptemp = ptemp->pnext)
    {
        if (ptemp == pnode)
        {
            return TRUE;
        }
    }

    return FALSE;
}

meshx_list_t *meshx_list_peek(const meshx_list_t *phead)
{
    if (phead->pnext == phead)
    {
        return NULL;
    }

    return phead->pnext;
}

/**
 * @brief get list length
 * @param head - list head
 * @return list length
 */
uint32_t meshx_list_length(const meshx_list_t *phead)
{
    MESHX_ASSERT(NULL != phead);

    uint32_t length = 0;
    meshx_list_t *ptemp = phead->pnext;
    for (; ptemp != phead; ptemp = ptemp->pnext)
    {
        ++ length;
    }

    return length;
}
