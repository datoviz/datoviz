/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph test helpers                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "frame_plan/frame_plan.h"
#include "_scene.h"
#include "scene_emit/scene_emit.h"
#include "_scene_shader_abi.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "render_contract/render_contract.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* out);
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TST_SCENE_GRAPH_GPU_FIXTURE "scene-graph-drp2-gpu"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TST_SCENE_GRAPH_GPU_CASE(test)                                                            \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_SCENE_GRAPH_SHARED_GPU_CASE(test)                                                     \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_SERIAL;                                               \
        _tst_desc.fixture = TST_SCENE_GRAPH_GPU_FIXTURE;                                          \
        _tst_desc.fixture_scope = TST_FIXTURE_SCOPE_PROCESS;                                      \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_SCENE_GRAPH_REQUIRE_VKLITE(ctx)                                                       \
    do                                                                                            \
    {                                                                                             \
        if (!_scene_vklite_runtime_available())                                                   \
        {                                                                                         \
            tst_skip((ctx), "Vulkan instance creation failed");                                   \
            return 0;                                                                             \
        }                                                                                         \
    } while (0)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

void _scene_graph_register_gpu_fixture(TstSuite* suite);
DvzDrp2Runtime* _scene_graph_fixture_runtime(TstContext* suite, DvzGpuCtx** out_gpu_ctx);
uint64_t _stream_scene_common_layout_id(const DvzDrp2CommandStream* stream);
uint64_t _stream_bind_group_layout_id(const DvzDrp2CommandStream* stream, uint64_t bind_group_id);
bool _stream_has_render_pipeline_label(const DvzDrp2CommandStream* stream, const char* label);
bool _stream_has_render_pipeline_label_part(const DvzDrp2CommandStream* stream, const char* part);
