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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/fileio.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/math/types.h"
#include "datoviz_testing.h"
#include "testing.h"

#if !defined(DVZ_SPIRV_ALIGN)
#if defined(__cplusplus)
#define DVZ_SPIRV_ALIGN alignas(uint32_t)
#else
#define DVZ_SPIRV_ALIGN _Alignas(uint32_t)
#endif
#endif

#ifndef DVZ_TEST_SPIRV_DIR
#error "DVZ_TEST_SPIRV_DIR must be defined when building vklite tests."
#endif

static inline void* dvz_test_shader_load(const char* filename, DvzSize* size_out)
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
        *size_out = shader_size;

    return shader;
}



/*************************************************************************************************/
/*  Tests vklite                                                                                 */
/*************************************************************************************************/

int test_vklite_commands_1(TstContext* suite, const TstCase* tstitem);
int test_vklite_commands_repeat_submit(TstContext* suite, const TstCase* tstitem);
int test_vklite_commands_destroy_idempotent(TstContext* suite, const TstCase* tstitem);
int test_vklite_timeline_wait_blocks_until_signal(TstContext* suite, const TstCase* tstitem);
int test_vklite_commands_destroy_without_recording(TstContext* suite, const TstCase* tstitem);
int test_vklite_commands_borrowed_recording_rejects_lifecycle(
    TstContext* suite, const TstCase* tstitem);
int test_vklite_commands_borrowed_recording_unwrap(TstContext* suite, const TstCase* tstitem);
int test_vklite_barriers_reset(TstContext* suite, const TstCase* tstitem);
int test_vklite_submit_reset_reuse(TstContext* suite, const TstCase* tstitem);

int test_vklite_sampler_1(TstContext* suite, const TstCase* tstitem);

int test_vklite_shader_1(TstContext* suite, const TstCase* tstitem);
int test_vklite_shader_create_requires_destroy(TstContext* suite, const TstCase* tstitem);

int test_vklite_slots_1(TstContext* suite, const TstCase* tstitem);
int test_vklite_slots_create_failure_unwinds_layouts(TstContext* suite, const TstCase* tstitem);

int test_vklite_compute_1(TstContext* suite, const TstCase* tstitem);
int test_vklite_compute_create_requires_destroy(TstContext* suite, const TstCase* tstitem);

int test_vklite_buffers_1(TstContext* suite, const TstCase* tstitem);

int test_vklite_buffer_views(TstContext* suite, const TstCase* tstitem);

int test_vklite_buffer_create_requires_destroy(TstContext* suite, const TstCase* tstitem);

int test_vklite_images_1(TstContext* suite, const TstCase* tstitem);

int test_vklite_images_create_requires_destroy(TstContext* suite, const TstCase* tstitem);

int test_vklite_descriptors_1(TstContext* suite, const TstCase* tstitem);
int test_vklite_rendering_reset(TstContext* suite, const TstCase* tstitem);

int test_vklite_graphics_1(TstContext* suite, const TstCase* tstitem);
int test_vklite_graphics_create_requires_destroy(TstContext* suite, const TstCase* tstitem);
int test_vklite_fixture_screenshot_repeat(TstContext* suite, const TstCase* tstitem);

int test_vklite_surface_query(TstContext* suite, const TstCase* tstitem);

int test_vklite_swapchain_recreate(TstContext* suite, const TstCase* tstitem);
int test_vklite_surface_swapchain_destroy_idempotent(TstContext* suite, const TstCase* tstitem);

int test_vklite_swapchain_config_present_mode_immediate(TstContext* suite, const TstCase* tstitem);

#if defined(VK_KHR_present_mode_fifo_latest_ready)
int test_vklite_swapchain_present_mode_fifo_latest_ready(
    TstContext* suite, const TstCase* tstitem);
#endif

int test_vklite_swapchain_config_defaults_partial(TstContext* suite, const TstCase* tstitem);

int test_vklite_swapchain_present_invalid_index(TstContext* suite, const TstCase* tstitem);

int test_vklite_swapchain_recreate_resolved_state(TstContext* suite, const TstCase* tstitem);
int test_vklite_swapchain_recreate_repeat_state(TstContext* suite, const TstCase* tstitem);
int test_vklite_swapchain_destroy_clears_cached_state(TstContext* suite, const TstCase* tstitem);
int test_vklite_swapchain_acquire_present_cycle(TstContext* suite, const TstCase* tstitem);

int test_vklite_wrap_backend_external_surface_present(TstContext* suite, const TstCase* tstitem);



/*************************************************************************************************/
/*  Tests techniques                                                                             */
/*************************************************************************************************/

int test_technique_triangle(TstContext* suite, const TstCase* tstitem);

int test_technique_render_texture(TstContext* suite, const TstCase* tstitem);

int test_technique_stencil(TstContext* suite, const TstCase* tstitem);

int test_technique_msaa(TstContext* suite, const TstCase* tstitem);

int test_technique_compute_graphics(TstContext* suite, const TstCase* tstitem);

int test_technique_picking(TstContext* suite, const TstCase* tstitem);

int test_technique_wboit(TstContext* suite, const TstCase* tstitem);



int test_vklite(TstSuite* suite);
