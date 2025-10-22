/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Memory management                                                                            */
/*************************************************************************************************/

#pragma once


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/math/arithm.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef void* (*DvzMallocFn)(DvzSize size);
typedef void* (*DvzCallocFn)(DvzSize count, DvzSize size);
typedef void* (*DvzReallocFn)(void* pointer, DvzSize size);
typedef void (*DvzFreeFn)(void* pointer);
typedef void* (*DvzAlignedAllocFn)(DvzSize alignment, DvzSize size);
typedef void (*DvzAlignedFreeFn)(void* pointer);

typedef struct DvzAllocator DvzAllocator;
typedef struct DvzPointer DvzPointer;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAllocator
{
    /* Primary heap allocation entry points. */
    DvzMallocFn malloc_fn;
    DvzCallocFn calloc_fn;
    DvzReallocFn realloc_fn;
    DvzFreeFn free_fn;
    /* Optional aligned allocation hooks; may be NULL for backends without support. */
    DvzAlignedAllocFn aligned_alloc_fn;
    DvzAlignedFreeFn aligned_free_fn;
};

struct DvzPointer
{
    void* pointer;
    /* On Windows aligned allocations must be paired with _aligned_free(). Keep the flag so that
     * callers can free through dvz_pointer_reset() without guessing the correct primitive. */
    bool aligned;
};



/*************************************************************************************************/
/*  Global allocator API                                                                         */
/*************************************************************************************************/

EXTERN_C_ON

DVZ_EXPORT void dvz_set_allocator(const DvzAllocator* allocator);
DVZ_EXPORT const DvzAllocator* dvz_get_allocator(void);
DVZ_EXPORT const DvzAllocator* dvz_system_allocator(void);
DVZ_EXPORT const DvzAllocator* dvz_mimalloc_allocator(void);
DVZ_EXPORT void dvz_use_system_allocator(void);
DVZ_EXPORT void dvz_use_mimalloc_allocator(void);

EXTERN_C_OFF



/*************************************************************************************************/
/*  Inline helpers                                                                               */
/*************************************************************************************************/

static inline const DvzAllocator* dvz_active_allocator(void)
{
    const DvzAllocator* allocator = dvz_get_allocator();
    ANN(allocator);
    return allocator;
}



static inline void* dvz_malloc(DvzSize size)
{
    /* Never pass 0 down to custom allocators; several implementations treat it as undefined. */
    if (size == 0)
        size = 1;
    const DvzAllocator* allocator = dvz_active_allocator();
    ANN(allocator->malloc_fn);
    return allocator->malloc_fn(size);
}



static inline void* dvz_calloc(DvzSize count, DvzSize size)
{
    /* Avoid requesting an empty allocation so downstream backends can assume size > 0. */
    if (count == 0 || size == 0)
    {
        count = count == 0 ? 1 : count;
        size = size == 0 ? 1 : size;
    }
    const DvzAllocator* allocator = dvz_active_allocator();
    ANN(allocator->calloc_fn);
    return allocator->calloc_fn(count, size);
}



static inline void* dvz_realloc(void* pointer, DvzSize size)
{
    /* Some allocators expect realloc(..., 0) to free the pointer; we honour the old behaviour
     * where size zero means "keep one byte alive" so callers do not trigger unexpected frees. */
    if (size == 0)
        size = 1;
    const DvzAllocator* allocator = dvz_active_allocator();
    ANN(allocator->realloc_fn);
    return allocator->realloc_fn(pointer, size);
}



static inline void dvz_free(void* pointer)
{
    if (pointer == NULL)
        return;
    const DvzAllocator* allocator = dvz_active_allocator();
    ANN(allocator->free_fn);
    allocator->free_fn(pointer);
}



static inline void* dvz_aligned_alloc(DvzSize alignment, DvzSize size)
{
    if (alignment == 0)
        return dvz_malloc(size);
    if (size == 0)
        size = 1;
    const DvzAllocator* allocator = dvz_active_allocator();
    if (allocator->aligned_alloc_fn == NULL)
    {
        log_error("aligned allocations are not supported by the active allocator");
        return NULL;
    }
    return allocator->aligned_alloc_fn(alignment, size);
}



static inline void dvz_aligned_free(void* pointer)
{
    if (pointer == NULL)
        return;
    const DvzAllocator* allocator = dvz_active_allocator();
    if (allocator->aligned_free_fn != NULL)
        allocator->aligned_free_fn(pointer);
    else
        dvz_free(pointer);
}



static inline void dvz_free_ptr(void** pointer)
{
    if (pointer == NULL || *pointer == NULL)
        return;
    dvz_free(*pointer);
    *pointer = NULL;
}



static inline void dvz_free_strings(uint32_t count, char** strings)
{
    if (strings == NULL)
        return;
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_free(strings[i]);
    }
    dvz_free(strings);
}



static inline void dvz_pointer_reset(DvzPointer* pointer)
{
    if (pointer == NULL || pointer->pointer == NULL)
        return;
    /* Match the allocation primitive that produced the pointer so the Windows CRT stays happy.
     * BIG FAT WARNING: never call dvz_free() directly on a pointer that originated from an aligned
     * allocator on Windows; always go through dvz_aligned_free() or this helper. */
    if (pointer->aligned)
        dvz_aligned_free(pointer->pointer);
    else
        dvz_free(pointer->pointer);
    pointer->pointer = NULL;
    pointer->aligned = false;
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static inline void* dvz_memdup(DvzSize size, const void* data)
{
    if (data == NULL || size == 0)
        return NULL;
    /* Replacement for the old _cpy() helper: copies arbitrary memory with the active allocator. */
    void* copy = dvz_malloc(size);
    ANN(copy);
    dvz_memcpy(copy, (size_t)size, data, (size_t)size);
    return copy;
}



static inline DvzSize dvz_alignment_get(DvzSize alignment, DvzSize min_alignment)
{
    if (alignment == 0)
        return min_alignment == 0 ? 0 : dvz_next_pow2(min_alignment);
    if (min_alignment > 0)
        alignment = (alignment + min_alignment - 1) & ~(min_alignment - 1);
    alignment = dvz_next_pow2(alignment);
    ASSERT(alignment >= min_alignment);
    return alignment;
}



static inline void* dvz_aligned_pointer(const void* data, DvzSize alignment, uint32_t idx)
{
    if (alignment == 0)
        return (void*)((const uint8_t*)data + idx);
    /* Return a pointer within a tightly-packed aligned buffer without recomputing layout. */
    return (void*)(((uint64_t)data) + ((uint64_t)idx * alignment));
}



static inline DvzSize dvz_aligned_size(DvzSize size, DvzSize alignment)
{
    if (alignment == 0)
        return size;
    ASSERT(alignment > 0);
    DvzSize remainder = size % alignment;
    if (remainder == 0)
        return size;
    return size + (alignment - remainder);
}



static inline DvzPointer
dvz_aligned_repeat(DvzSize size, const void* data, uint32_t count, DvzSize alignment)
{
    DvzSize item_size = alignment > 0 ? dvz_alignment_get(size, alignment) : size;
    DvzSize total_size = item_size * count;
    /* Back-port of aligned_repeat(): duplicate a small pattern in an aligned heap buffer. */
    void* repeated =
        alignment > 0 ? dvz_aligned_alloc(alignment, total_size) : dvz_malloc(total_size);
    ANN(repeated);
    dvz_memset(repeated, (size_t)total_size, 0, (size_t)total_size);
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_memcpy(
            (void*)((uint8_t*)repeated + ((size_t)i * (size_t)item_size)), (size_t)size, data,
            (size_t)size);
    }
    /* WARNING: the returned pointer carries the aligned flag so callers free it correctly on all
     * platforms (Windows requires _aligned_free for true aligned blocks). */
    DvzPointer out = {0};
    out.pointer = repeated;
    out.aligned = alignment > 0;
    return out;
}
