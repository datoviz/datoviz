/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing vklite                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/fileio.h"
#include "_alloc.h"
#include "_compat.h"
#include "testing.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DVZ_TEST_SPIRV_DIR
#error "DVZ_TEST_SPIRV_DIR must be defined when building vklite tests."
#endif

static inline void* dvz_test_shader_load(const char* filename, size_t* size_out)
{
    if (!filename)
        return NULL;

    const char* shader_dir = DVZ_TEST_SPIRV_DIR;
    size_t dir_len = strlen(shader_dir);
    size_t name_len = strlen(filename);
    size_t path_len = dir_len + 1 + name_len + 1;

    char* path = (char*)dvz_malloc(path_len);
    if (!path)
        return NULL;

    int written = dvz_snprintf(path, path_len, "%s/%s", shader_dir, filename);
    if (written < 0 || (size_t)written >= path_len)
    {
        dvz_free(path);
        return NULL;
    }

    DvzSize shader_size = 0;
    void* shader = dvz_read_file(path, &shader_size);
    dvz_free(path);
    if (!shader)
        return NULL;

    if (size_out)
        *size_out = (size_t)shader_size;

    return shader;
}


/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_commands_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_sampler_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_shader_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_slots_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_compute_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_buffers_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_buffer_views(TstSuite* suite, TstItem* tstitem);

int test_vklite_images_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_descriptors_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_graphics_1(TstSuite* suite, TstItem* tstitem);



int test_vklite(TstSuite* suite);
