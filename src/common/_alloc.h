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
#include <stdlib.h>
#include <string.h>

#include "_assert.h"
#include "_log.h"
#include "datoviz/math/arithm.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define FREE(x)                                                                                   \
    if ((x) != NULL)                                                                              \
    {                                                                                             \
        free((x));                                                                                \
        (x) = NULL;                                                                               \
    }

#define ALIGNED_FREE(x)                                                                           \
    if (x.aligned)                                                                                \
        aligned_free(x.pointer);                                                                  \
    else                                                                                          \
        FREE(x.pointer)

#define REALLOC(T, x, s)                                                                          \
    {                                                                                             \
        T _new = (T)realloc((x), (s));                                                            \
        if (_new == NULL)                                                                         \
            exit(1);                                                                              \
        else                                                                                      \
            x = _new;                                                                             \
    }



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPointer DvzPointer;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPointer
{
    void* pointer;
    bool aligned;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// NOTE: the returned pointer will have to be freed.
static void* _cpy(DvzSize size, const void* data)
{
    if (data == NULL)
        return NULL;
    void* data_cpy = malloc(size);
    memcpy(data_cpy, data, size);
    return data_cpy;
}



static DvzSize get_alignment(DvzSize alignment, DvzSize min_alignment)
{
    if (min_alignment > 0)
        alignment = (alignment + min_alignment - 1) & ~(min_alignment - 1);
    alignment = dvz_next_pow2(alignment);
    ASSERT(alignment >= min_alignment);
    return alignment;
}



/*
BIG FAT WARNING: never FREE a pointer returned by aligned_malloc(), use aligned_free() instead.
Otherwise direct crash on Windows.
*/
static void* aligned_malloc(DvzSize size, DvzSize alignment)
{
    void* data = NULL;
    // Allocate the aligned buffer.
#if OS_MACOS
    posix_memalign((void**)&data, alignment, size);
#elif OS_WINDOWS
    data = _aligned_malloc(size, alignment);
#else
    data = aligned_alloc(alignment, size);
#endif

    if (data == NULL)
        log_error("failed making the aligned allocation of the dynamic uniform buffer");
    return data;
}



/*
WARNING : on Windows, only works on aligned pointers. */
static void aligned_free(void* pointer)
{
#if OS_WINDOWS
    _aligned_free(pointer);
#else
    FREE(pointer)
#endif
}



static void* aligned_pointer(const void* data, DvzSize alignment, uint32_t idx)
{
    // Get a pointer to a given item in the dynamic uniform buffer, to update it.
    return (void*)(((uint64_t)data + (idx * alignment)));
}



static DvzSize aligned_size(DvzSize size, DvzSize alignment)
{
    if (alignment == 0)
        return size;
    ASSERT(alignment > 0);
    if (size % alignment == 0)
        return size;
    ASSERT(size % alignment < alignment);
    size += (alignment - (size % alignment));
    ASSERT(size % alignment == 0);
    return size;
}



/*
WARNING: returns a wrapped pointer specifiying whether the pointer was aligned or not.
This is needed on Windows because aligned pointers must be freed with _aligned_free(), whereas
normal pointers must be freed with free(). Without a wrapper DvzPointer struct, this function
wouldn't be able to say whether its returned pointer was aligned-allocated or normally allocated.
*/
static DvzPointer aligned_repeat(DvzSize size, const void* data, uint32_t count, DvzSize alignment)
{
    // Take any buffer and make `count` consecutive aligned copies of it.
    // The caller must free the result.
    DvzSize alsize = alignment > 0 ? get_alignment(size, alignment) : size;
    DvzSize rep_size = alsize * count;
    void* repeated = NULL;
    if (alignment > 0)
        repeated = aligned_malloc(rep_size, alignment);
    else
        repeated = malloc(rep_size);
    ANN(repeated);
    memset(repeated, 0, rep_size);
    for (uint32_t i = 0; i < count; i++)
    {
        memcpy((void*)(((int64_t)repeated) + (int64_t)(i * alsize)), data, size);
    }
    DvzPointer out = {0, 0};
    out.pointer = repeated;
    out.aligned = alignment > 0;
    return out;
}
