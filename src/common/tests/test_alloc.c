/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing allocator                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "test_common.h"
#include "testing.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_alloc_basic(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    const DvzAllocator* allocator = dvz_get_allocator();
    AT(allocator != NULL);

    // Basic malloc/free
    uint8_t* buffer = (uint8_t*)dvz_malloc(128);
    AT(buffer != NULL);
    for (uint32_t i = 0; i < 128; i++)
        buffer[i] = (uint8_t)i;
    dvz_free(buffer);

    // calloc zero initialisation and realloc growth
    uint32_t* values = (uint32_t*)dvz_calloc(4, sizeof(uint32_t));
    AT(values != NULL);
    for (uint32_t i = 0; i < 4; i++)
        AT(values[i] == 0);

    values[0] = 42;
    values = (uint32_t*)dvz_realloc(values, 16 * sizeof(uint32_t));
    AT(values != NULL);
    AT(values[0] == 42);
    dvz_free(values);

    // dvz_free_ptr helper clears pointer
    uint8_t* temp = (uint8_t*)dvz_malloc(8);
    dvz_free_ptr((void**)&temp);
    AT(temp == NULL);

    return 0;
}



int test_alloc_aligned(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    const DvzSize alignment = 64;
    const DvzSize size = 512;
    uint8_t* aligned = (uint8_t*)dvz_aligned_alloc(alignment, size);
    AT(aligned != NULL);
    AT(((uintptr_t)aligned % alignment) == 0);
    dvz_memset(aligned, (size_t)size, 0xAB, (size_t)size);
    dvz_aligned_free(aligned);

    // Verify dvz_aligned_repeat duplicates data correctly.
    uint8_t pattern[16];
    for (size_t i = 0; i < sizeof(pattern); i++)
        pattern[i] = (uint8_t)(i * 3);

    DvzPointer repeated = dvz_aligned_repeat(sizeof(pattern), pattern, 4, alignment);
    AT(repeated.pointer != NULL);
    AT(repeated.aligned);

    uint8_t* repeated_bytes = (uint8_t*)repeated.pointer;
    DvzSize stride = dvz_alignment_get(sizeof(pattern), alignment);
    for (uint32_t block = 0; block < 4; block++)
    {
        size_t offset = (size_t)block * stride;
        AT(memcmp(&repeated_bytes[offset], pattern, sizeof(pattern)) == 0);
    }

    dvz_pointer_reset(&repeated);
    AT(repeated.pointer == NULL);

    return 0;
}
