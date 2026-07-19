/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 test helpers                                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2.h"
#include "datoviz/stream/frame_stream.h"
#include "testing.h"

#if DVZ_DRP2_HAS_VKLITE
#include "datoviz/vk/gpu_ctx.h"
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TST_DRP2_VKLITE_FIXTURE "drp2-vklite-runtime"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

bool drp2_test_captured_log_contains(const TstContext* suite, const char* needle);

bool drp2_test_create_render_pipeline(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots);

bool drp2_test_create_render_pipeline_with_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots,
    uint64_t bind_group_layout_id);

DvzDrp2CommandStream* drp2_test_valid_render_stream(void);

DvzDrp2CommandStream* drp2_test_valid_indexed_render_stream(void);

DvzDrp2CommandStream* drp2_test_valid_compute_stream(void);

DvzStreamFrame drp2_test_stream_frame(uintptr_t seed, uint32_t width, uint32_t height);

#if DVZ_DRP2_HAS_VKLITE
bool drp2_test_vklite_runtime_available(void);

void* drp2_test_vklite_fixture_create(TstSuite* suite, uint32_t worker_index);

void drp2_test_vklite_fixture_destroy(void* fixture_ptr);

DvzDrp2Runtime* drp2_test_vklite_fixture_runtime(TstContext* suite, DvzGpuCtx** out_gpu_ctx);

int drp2_test_vklite_validation_setup(TstContext* suite, const TstCase* item);

int drp2_test_vklite_validation_teardown(TstContext* suite, const TstCase* item);

bool drp2_test_vklite_validation_clean(TstContext* suite, DvzGpuCtx* gpu_ctx);
#endif
