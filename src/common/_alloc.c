/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Allocator implementation                                                                     */
/*************************************************************************************************/

#include "_alloc.h"

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>

#if OS_WINDOWS
#include <malloc.h>
#endif

#if defined(DVZ_HAS_MIMALLOC) && DVZ_HAS_MIMALLOC
#include "mimalloc.h"
#endif

/* Guard macros let us compile this file even when mimalloc is absent; they are resolved by
 * CMake so the Release build can opt-in without penalising Debug or custom setups. */
#ifndef DVZ_HAS_MIMALLOC
#define DVZ_HAS_MIMALLOC 0
#endif

#ifndef DVZ_ALLOCATOR_DEFAULT_MIMALLOC
#define DVZ_ALLOCATOR_DEFAULT_MIMALLOC 0
#endif



/*************************************************************************************************/
/*  System allocator                                                                             */
/*************************************************************************************************/

static void* dvz_system_malloc(DvzSize size)
{
    /* Plain old libc malloc. */
    return malloc((size_t)size);
}



static void* dvz_system_calloc(DvzSize count, DvzSize size)
{
    return calloc((size_t)count, (size_t)size);
}



static void* dvz_system_realloc(void* pointer, DvzSize size)
{
    return realloc(pointer, (size_t)size);
}



static void* dvz_system_aligned_alloc(DvzSize alignment, DvzSize size)
{
    alignment = dvz_alignment_get(alignment, sizeof(void*));
    DvzSize aligned_size = dvz_aligned_size(size, alignment);

#if OS_WINDOWS
    void* data = _aligned_malloc((size_t)aligned_size, (size_t)alignment);
    if (data == NULL)
        log_error(
            "failed aligned allocation (size=%" PRIu64 ", alignment=%" PRIu64 ")", size,
            alignment);
    return data;
#else
    void* data = NULL;
#if defined(_ISOC11_SOURCE) || (__STDC_VERSION__ >= 201112L && !defined(OS_MACOS))
    data = aligned_alloc((size_t)alignment, (size_t)aligned_size);
    if (data == NULL)
        log_error(
            "failed aligned allocation (size=%" PRIu64 ", alignment=%" PRIu64 ")", size,
            alignment);
    return data;
#else
    int rc = posix_memalign(&data, (size_t)alignment, (size_t)aligned_size);
    if (rc != 0)
    {
        log_error(
            "posix_memalign failed (rc=%d, size=%" PRIu64 ", alignment=%" PRIu64 ")", rc, size,
            alignment);
        data = NULL;
    }
    return data;
#endif
#endif
}



static void dvz_system_free(void* pointer)
{
    if (pointer != NULL)
        free(pointer);
}



static void dvz_system_aligned_free(void* pointer)
{
    if (pointer == NULL)
        return;
#if OS_WINDOWS
    _aligned_free(pointer);
#else
    free(pointer);
#endif
}



static const DvzAllocator DVZ_SYSTEM_ALLOCATOR = {
    /* Fallback backend: rely on the platform C runtime. */
    .malloc_fn = dvz_system_malloc,
    .calloc_fn = dvz_system_calloc,
    .realloc_fn = dvz_system_realloc,
    .free_fn = dvz_system_free,
    .aligned_alloc_fn = dvz_system_aligned_alloc,
    .aligned_free_fn = dvz_system_aligned_free,
};



/*************************************************************************************************/
/*  Mimalloc allocator                                                                           */
/*************************************************************************************************/

#if DVZ_HAS_MIMALLOC

static void* dvz_mimalloc_malloc(DvzSize size) { return mi_malloc((size_t)size); }



static void* dvz_mimalloc_calloc(DvzSize count, DvzSize size)
{
    return mi_calloc((size_t)count, (size_t)size);
}



static void* dvz_mimalloc_realloc(void* pointer, DvzSize size)
{
    return mi_realloc(pointer, (size_t)size);
}



static void* dvz_mimalloc_aligned_alloc(DvzSize alignment, DvzSize size)
{
    alignment = dvz_alignment_get(alignment, sizeof(void*));
    DvzSize aligned_size = dvz_aligned_size(size, alignment);
    void* data = mi_malloc_aligned((size_t)aligned_size, (size_t)alignment);
    if (data == NULL)
        log_error(
            "mi_malloc_aligned failed (size=%" PRIu64 ", alignment=%" PRIu64 ")", size, alignment);
    return data;
}



static void dvz_mimalloc_free(void* pointer) { mi_free(pointer); }



static const DvzAllocator DVZ_MIMALLOC_ALLOCATOR = {
    /* Release default: wrap mimalloc so the rest of the codebase keeps a stable API surface. */
    .malloc_fn = dvz_mimalloc_malloc,
    .calloc_fn = dvz_mimalloc_calloc,
    .realloc_fn = dvz_mimalloc_realloc,
    .free_fn = dvz_mimalloc_free,
    .aligned_alloc_fn = dvz_mimalloc_aligned_alloc,
    .aligned_free_fn = dvz_mimalloc_free,
};

#endif



/*************************************************************************************************/
/*  Global allocator state                                                                       */
/*************************************************************************************************/

static const DvzAllocator* DVZ_ACTIVE_ALLOCATOR = NULL;



static void dvz_allocator_set_default(void)
{
#if DVZ_ALLOCATOR_DEFAULT_MIMALLOC && DVZ_HAS_MIMALLOC
    DVZ_ACTIVE_ALLOCATOR = &DVZ_MIMALLOC_ALLOCATOR;
#else
    DVZ_ACTIVE_ALLOCATOR = &DVZ_SYSTEM_ALLOCATOR;
#endif
}



void dvz_set_allocator(const DvzAllocator* allocator)
{
    if (allocator == NULL)
    {
        /* A NULL request reverts to the configuration-time default. */
        dvz_allocator_set_default();
        return;
    }
    DVZ_ACTIVE_ALLOCATOR = allocator;
}



const DvzAllocator* dvz_get_allocator(void)
{
    if (DVZ_ACTIVE_ALLOCATOR == NULL)
        dvz_allocator_set_default();
    return DVZ_ACTIVE_ALLOCATOR;
}



const DvzAllocator* dvz_system_allocator(void) { return &DVZ_SYSTEM_ALLOCATOR; }



const DvzAllocator* dvz_mimalloc_allocator(void)
{
#if DVZ_HAS_MIMALLOC
    return &DVZ_MIMALLOC_ALLOCATOR;
#else
    return NULL;
#endif
}



void dvz_use_system_allocator(void) { dvz_set_allocator(dvz_system_allocator()); }



void dvz_use_mimalloc_allocator(void)
{
#if DVZ_HAS_MIMALLOC
    /* Caller asked for mimalloc at runtime: update the global handle. */
    dvz_set_allocator(dvz_mimalloc_allocator());
#else
    log_warn("mimalloc allocator requested but mimalloc is not available");
#endif
}
