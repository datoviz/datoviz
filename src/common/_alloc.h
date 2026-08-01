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
#include "_overflow.h"
#include "datoviz/common/macros.h"
#include "datoviz/math/arithm.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_STRING_LENGTH 4096

#define POINTER_OFFSET(x, o) (void*)((uint64_t)(x) + (uint64_t)(o))



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

/* Portable fallbacks for strlcpy/strlcat on platforms that don't provide them (e.g., MSVC).
 * Implementations follow typical BSD semantics: return the length of the string it tried to
 * create. Map to the standard names via macros if they are not already defined so existing code
 * using strlcpy/strlcat compiles on all platforms. */
static inline size_t dvz_strlcpy(char* dst, const char* src, size_t siz)
{
    size_t src_len = src ? strlen(src) : 0;
    if (siz)
    {
        size_t to_copy = (src_len >= siz) ? (siz - 1) : src_len;
        if (to_copy && dst && src)
            dvz_memcpy(dst, siz, src, to_copy);
        if (dst)
            dst[to_copy] = '\0';
    }
    return src_len;
}

static inline size_t dvz_strlcat(char* dst, const char* src, size_t siz)
{
    size_t dst_len = dst ? strlen(dst) : 0;
    size_t src_len = src ? strlen(src) : 0;

    if (dst_len >= siz)
        return siz + src_len;

    size_t space = siz - dst_len - 1; /* remaining space excluding NUL */
    size_t to_copy = (src_len > space) ? space : src_len;
    if (to_copy && dst && src)
        dvz_memcpy(dst + dst_len, siz - dst_len, src, to_copy);
    if (dst)
        dst[dst_len + to_copy] = '\0';
    return dst_len + src_len;
}

#if !defined(HAVE_STRLCPY)
#if !defined(strlcpy)
#define strlcpy dvz_strlcpy
#endif
#endif

#if !defined(HAVE_STRLCAT)
#if !defined(strlcat)
#define strlcat dvz_strlcat
#endif
#endif

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
    if (size > SIZE_MAX)
    {
        log_error("allocation size exceeds the platform limit");
        return NULL;
    }
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
    uint64_t total_size = 0;
    if (_dvz_mul_u64_overflows(count, size, &total_size) || total_size > SIZE_MAX)
    {
        log_error("allocation size multiplication overflow");
        return NULL;
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
    if (size > SIZE_MAX)
    {
        log_error("reallocation size exceeds the platform limit");
        return NULL;
    }
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
    if (alignment > SIZE_MAX || size > SIZE_MAX)
    {
        log_error("aligned allocation exceeds the platform limit");
        return NULL;
    }
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
/*  Strings */
/*************************************************************************************************/

static inline void dvz_copy_strings(uint32_t count, const char** src, char** dst)
{
    // Assumes src and dst are already allocated.
    if (count == 0 || src == NULL)
        return;

    if (!dst)
        return;

    for (uint32_t i = 0; i < count; i++)
    {
        const char* s = src[i];
        if (s)
        {
            const size_t len = strlen(s);
            if (len == SIZE_MAX)
            {
                log_error("string length overflow");
                continue;
            }
            char* copy = (char*)dvz_calloc(len + 1, sizeof(char));
            if (copy)
            {
                dvz_memcpy(copy, len + 1, s, len + 1);
                dst[i] = copy;
            }
        }
        else
        {
            dst[i] = NULL;
        }
    }
}



static inline bool dvz_strings_contains(uint32_t count, char** strings, const char* string)
{
    if (count == 0)
        return false;
    if (strings == NULL || string == NULL)
        return false;
    ASSERT(count > 0);
    ANN(strings);

    for (uint32_t i = 0; i < count; i++)
    {
        if (strings[i] != NULL && strcmp(strings[i], string) == 0)
            return true;
    }
    return false;
}



static inline void dvz_strings_show(uint32_t count, char** strings)
{
    ANN(strings);
    for (uint32_t i = 0; i < count; i++)
    {
        log_info("  - %s", strings[i] != NULL ? strings[i] : "(null)");
    }
}



static inline void dvz_free_strings(uint32_t count, char** strings)
{
    if (strings == NULL)
        return;
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_free(strings[i]);
    }

    // NOTE: the caller must free the array of strings itself.
    // dvz_free(strings);
}

#define DVZ_FREE_STRING_CONTAINER(count, strings)                                                 \
    if ((count) > 0)                                                                              \
    {                                                                                             \
        ANN((strings));                                                                           \
        dvz_free_strings(count, (strings));                                                       \
        dvz_free((strings));                                                                      \
    }                                                                                             \
    else                                                                                          \
    {                                                                                             \
        ASSERT(strings == NULL);                                                                  \
    }                                                                                             \
    strings = NULL;



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static inline void* dvz_memdup(DvzSize size, const void* data)
{
    if (data == NULL || size == 0 || size > SIZE_MAX)
        return NULL;
    /* Replacement for the old _cpy() helper: copies arbitrary memory with the active allocator. */
    void* copy = dvz_malloc(size);
    ANN(copy);
    dvz_memcpy(copy, (size_t)size, data, (size_t)size);
    return copy;
}



static inline char* dvz_strdup(const char* s)
{
    if (s == NULL)
        return NULL;

    const size_t len = strlen(s);
    if (len == SIZE_MAX)
        return NULL;
    char* copy = (char*)dvz_malloc(len + 1);
    if (copy != NULL)
        dvz_memcpy(copy, len + 1, s, len + 1);
    return copy;
}



static inline DvzSize dvz_alignment_get(DvzSize alignment, DvzSize min_alignment)
{
    const DvzSize requested = alignment > min_alignment ? alignment : min_alignment;
    if (requested == 0)
        return 0;
    if (requested > (UINT64_C(1) << 63u))
    {
        log_error("alignment exceeds the supported power-of-two range");
        return 0;
    }
    return dvz_next_pow2(requested);
}



static inline const void* dvz_aligned_pointer(const void* data, DvzSize alignment, uint32_t idx)
{
    if (alignment == 0)
        return (const void*)((const uint8_t*)data + idx);

    return (const void*)((uintptr_t)data + (uintptr_t)idx * alignment);
}



static inline DvzSize dvz_aligned_size(DvzSize size, DvzSize alignment)
{
    if (alignment == 0)
        return size;
    ASSERT(alignment > 0);
    DvzSize remainder = size % alignment;
    if (remainder == 0)
        return size;
    DvzSize aligned_size = 0;
    if (_dvz_add_u64_overflows(size, alignment - remainder, &aligned_size))
    {
        log_error("aligned size overflow");
        return 0;
    }
    return aligned_size;
}



static inline DvzPointer
dvz_aligned_repeat(DvzSize size, const void* data, uint32_t count, DvzSize alignment)
{
    INIT(DvzPointer, out);
    if (size == 0 || data == NULL || count == 0)
        return out;
    DvzSize item_size = alignment > 0 ? dvz_alignment_get(size, alignment) : size;
    if (item_size == 0)
        return out;
    uint64_t total_size = 0;
    if (_dvz_mul_u64_overflows(item_size, count, &total_size) || total_size > SIZE_MAX)
    {
        log_error("aligned repeat size overflow");
        return out;
    }
    /* Back-port of aligned_repeat(): duplicate a small pattern in an aligned heap buffer. */
    void* repeated =
        alignment > 0 ? dvz_aligned_alloc(alignment, total_size) : dvz_malloc(total_size);
    if (repeated == NULL)
        return out;
    dvz_memset(repeated, (size_t)total_size, 0, (size_t)total_size);
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_memcpy(
            (void*)((uint8_t*)repeated + ((size_t)i * (size_t)item_size)), (size_t)size, data,
            (size_t)size);
    }
    /* WARNING: the returned pointer carries the aligned flag so callers free it correctly on all
     * platforms (Windows requires _aligned_free for true aligned blocks). */
    out.pointer = repeated;
    out.aligned = alignment > 0;
    return out;
}
