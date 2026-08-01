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
#include "datoviz/common/functions.h"
#include "test_common.h"
#include "testing.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_alloc_basic(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    const DvzAllocator* allocator = dvz_get_allocator();
    AT(allocator != NULL);

    // Basic malloc/free
    uint8_t* buffer = (uint8_t*)dvz_malloc(128);
    AT(buffer != NULL);
    for (uint32_t i = 0; i < 128; i++)
        buffer[i] = (uint8_t)i;
    dvz_memory_free(buffer);
    dvz_memory_free(NULL);
    AT(dvz_error_set_callback(NULL, NULL) == DVZ_OK);

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

    // String helpers preserve long values and tolerate sparse string arrays.
    char long_string[DVZ_MAX_STRING_LENGTH + 257] = {0};
    dvz_memset(long_string, sizeof(long_string), 'x', sizeof(long_string) - 1);
    char* duplicate = dvz_strdup(long_string);
    AT(duplicate != NULL);
    AT(strlen(duplicate) == sizeof(long_string) - 1);
    AT(strcmp(duplicate, long_string) == 0);
    dvz_free(duplicate);

    const char* source_strings[3] = {"short", NULL, long_string};
    char* copied_strings[3] = {0};
    dvz_copy_strings(3, source_strings, copied_strings);
    AT(copied_strings[0] != NULL);
    AT(copied_strings[1] == NULL);
    AT(copied_strings[2] != NULL);
    AT(strcmp(copied_strings[2], long_string) == 0);
    AT(dvz_strings_contains(3, copied_strings, long_string));
    AT(!dvz_strings_contains(3, copied_strings, "missing"));
    AT(!dvz_strings_contains(3, copied_strings, NULL));
    dvz_free_strings(3, copied_strings);

    return 0;
}



int test_alloc_aligned(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    const DvzSize alignment = 64;
    const DvzSize size = 512;
    uint8_t* aligned = (uint8_t*)dvz_aligned_alloc(alignment, size);
    AT(aligned != NULL);
    AT(((uintptr_t)aligned % alignment) == 0);
    dvz_memset(aligned, (size_t)size, 0xAB, (size_t)size);
    dvz_aligned_free(aligned);

    AT(dvz_alignment_get(16, 64) == 64);
    AT(dvz_alignment_get(80, 64) == 128);
    AT(dvz_alignment_get(16, 48) == 64);

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

    DvzPointer empty = dvz_aligned_repeat(0, pattern, 4, alignment);
    AT(empty.pointer == NULL);
    log_set_quiet(1);
    DvzPointer overflow = dvz_aligned_repeat(UINT64_MAX / 2u + 1u, pattern, 2, 0);
    log_set_quiet(0);
    AT(overflow.pointer == NULL);

    return 0;
}
