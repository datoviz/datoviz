/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Allocation algorithm                                                                         */
/*************************************************************************************************/

#include "alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



typedef struct Block
{
    DvzSize offset;
    DvzSize size;
    bool free;
    struct Block* next;
} Block;



struct DvzAlloc
{
    DvzSize total_size;
    DvzSize alignment;
    Block* blocks;
    DvzSize allocated_size;
};



static Block* create_block(DvzSize offset, DvzSize size, bool free)
{
    Block* block = (Block*)malloc(sizeof(Block));
    ANN(block);
    block->offset = offset;
    block->size = size;
    block->free = free;
    block->next = NULL;
    return block;
}



DvzAlloc* dvz_alloc(DvzSize size, DvzSize alignment)
{
    DvzAlloc* alloc = (DvzAlloc*)malloc(sizeof(DvzAlloc));
    ANN(alloc);
    alloc->total_size = size;
    alloc->alignment = alignment;
    alloc->allocated_size = 0;
    alloc->blocks = create_block(0, size, 1);
    return alloc;
}



DvzSize dvz_alloc_new(DvzAlloc* alloc, DvzSize req_size, DvzSize* resized)
{
    ANN(alloc);
    if (req_size == 0)
    {
        log_error("requested allocation size must be >0");
        return 0;
    }

    DvzSize aligned_size = _align(req_size, alloc->alignment);
    ASSERT(aligned_size > 0);
    Block* current = alloc->blocks;
    ANN(current);

    while (current != NULL)
    {
        if (current->free && current->size >= aligned_size)
        {
            if (current->size > aligned_size)
            {
                Block* new_block =
                    create_block(current->offset + aligned_size, current->size - aligned_size, 1);
                ANN(new_block);
                new_block->next = current->next;
                current->next = new_block;
            }
            current->size = aligned_size;
            current->free = false;
            alloc->allocated_size += aligned_size;
            return current->offset;
        }
        current = current->next;
    }

    DvzSize new_size = alloc->total_size * 2;
    ASSERT(new_size > 0);
    if (resized != NULL)
        *resized = new_size;

    Block* new_block = create_block(alloc->total_size, new_size - alloc->total_size, 1);
    ANN(new_block);
    current = alloc->blocks;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = new_block;
    alloc->total_size = new_size;
    return dvz_alloc_new(alloc, req_size, resized);
}



void dvz_alloc_free(DvzAlloc* alloc, DvzSize offset)
{
    ANN(alloc);

    Block* current = alloc->blocks;

    while (current != NULL)
    {
        if (current->offset == offset)
        {
            if (current->free)
            {
                log_error("should not free an already-freed chunk");
            }
            current->free = true;
            ASSERT(alloc->allocated_size >= current->size);
            alloc->allocated_size -= current->size;
            break;
        }
        current = current->next;
    }

    current = alloc->blocks;
    while (current != NULL && current->next != NULL)
    {
        if (current->free && current->next->free)
        {
            Block* next_block = current->next;
            ANN(next_block);
            current->size += next_block->size;
            current->next = next_block->next;
            FREE(next_block);
        }
        else
        {
            current = current->next;
        }
    }
}



DvzSize dvz_alloc_get(DvzAlloc* alloc, DvzSize offset)
{
    ANN(alloc);
    Block* current = alloc->blocks;

    while (current != NULL)
    {
        if (current->offset == offset)
        {
            return current->size;
        }
        current = current->next;
    }
    return 0;
}



void dvz_alloc_size(DvzAlloc* alloc, DvzSize* out_alloc, DvzSize* out_total)
{
    ANN(alloc);
    if (out_alloc)
        *out_alloc = alloc->allocated_size;
    if (out_total)
        *out_total = alloc->total_size;
}



void dvz_alloc_stats(DvzAlloc* alloc)
{
    ANN(alloc);

    printf("Total size: %s\n", pretty_size(alloc->total_size));
    printf(
        "Allocated size: %s (%.1f%%)\n", //
        pretty_size(alloc->allocated_size), alloc->allocated_size * 100.0 / alloc->total_size);
}



void dvz_alloc_clear(DvzAlloc* alloc)
{
    ANN(alloc);

    Block* current = alloc->blocks;

    while (current != NULL)
    {
        Block* next = current->next;
        FREE(current);
        current = next;
    }

    alloc->blocks = create_block(0, alloc->total_size, 1);
    alloc->allocated_size = 0;
}



void dvz_alloc_destroy(DvzAlloc* alloc)
{
    ANN(alloc);

    dvz_alloc_clear(alloc);
    FREE(alloc->blocks);
    FREE(alloc);
}
