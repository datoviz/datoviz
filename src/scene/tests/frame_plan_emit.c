/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan emission tests                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_scene.h"
#include "_technique.h"
#include "../../drp2/_stream.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"
#include "datoviz/window.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* out);
#endif




/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

typedef enum
{
    SCENE_DVZR_VISUAL_POINT,
    SCENE_DVZR_VISUAL_PRIMITIVE,
    SCENE_DVZR_VISUAL_MESH,
    SCENE_DVZR_VISUAL_IMAGE,
} SceneDvzrVisualKind;



/**
 * Attach explicit typed metadata for a generic fixture-render visual.
 *
 * @param plan the FramePlan
 * @param visual_id render visual id
 * @param position_id position buffer resource id
 * @return whether the visual and metadata were attached
 */
static bool _frame_plan_render_fixture_visual(
    DvzFramePlan* plan, const char* visual_id, const char* position_id)
{
    ANN(plan);
    ANN(visual_id);
    ANN(position_id);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, position_id, sizeof(metadata.position_id));
    return dvz_frame_plan_render_visual(plan, visual_id) &&
           dvz_frame_plan_render_visual_metadata(plan, &metadata);
}



#define TST_SCENE_GPU_CASE(test)                                                                  \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)


#define TST_SCENE_FRAME_PLAN_GPU_FIXTURE "scene-frame-plan-drp2-gpu"

#define TST_SCENE_SHARED_GPU_CASE(test)                                                           \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_SERIAL;                                               \
        _tst_desc.fixture = TST_SCENE_FRAME_PLAN_GPU_FIXTURE;                                     \
        _tst_desc.fixture_scope = TST_FIXTURE_SCOPE_PROCESS;                                      \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)


#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
typedef struct
{
    DvzGpuCtx* gpu_ctx;
    DvzDrp2Runtime* runtime;
    bool available;
    const char* skip_reason;
} SceneFramePlanGpuFixture;



typedef struct
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    uint64_t texture_id;
} SceneDepthPeelRuntimeTarget;



typedef struct
{
    SceneDepthPeelRuntimeTarget targets[DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY];
    uint32_t count;
} SceneDepthPeelRuntimeTargets;
#endif



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
/**
 * Return the GPU context configuration for shared frame-plan DRP2 execution tests.
 *
 * @return GPU context configuration with required Vulkan 1.3 features
 */
static DvzGpuCtxConfig _scene_frame_plan_gpu_ctx_config(void)
{
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    return gpu_cfg;
}



/**
 * Create a process-scoped GPU/runtime fixture for frame-plan DRP2 execution tests.
 *
 * @param suite active test suite
 * @param worker_index scheduler worker index
 * @return fixture state
 */
static void* _scene_frame_plan_gpu_fixture_create(TstSuite* suite, uint32_t worker_index)
{
    (void)suite;
    (void)worker_index;

    SceneFramePlanGpuFixture* fixture =
        (SceneFramePlanGpuFixture*)dvz_calloc(1, sizeof(SceneFramePlanGpuFixture));
    ANN(fixture);

    if (!_scene_vklite_runtime_available())
    {
        fixture->skip_reason = "Vulkan instance creation failed";
        return fixture;
    }

    DvzGpuCtxConfig gpu_cfg = _scene_frame_plan_gpu_ctx_config();
    fixture->gpu_ctx = dvz_gpu_ctx(&gpu_cfg);
    if (fixture->gpu_ctx == NULL)
    {
        fixture->skip_reason = "GPU context creation failed";
        return fixture;
    }

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(
        dvz_gpu_ctx_device(fixture->gpu_ctx), dvz_gpu_ctx_alloc(fixture->gpu_ctx));
    fixture->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (fixture->runtime == NULL)
    {
        fixture->skip_reason = "DRP2 runtime creation failed";
        return fixture;
    }

    fixture->available = true;
    return fixture;
}



/**
 * Destroy the process-scoped frame-plan DRP2 GPU fixture.
 *
 * @param fixture_ptr fixture state
 */
static void _scene_frame_plan_gpu_fixture_destroy(void* fixture_ptr)
{
    SceneFramePlanGpuFixture* fixture = (SceneFramePlanGpuFixture*)fixture_ptr;
    if (fixture == NULL)
        return;
    if (fixture->gpu_ctx != NULL)
    {
        DvzInstance* instance = dvz_gpu_ctx_instance(fixture->gpu_ctx);
        if (instance != NULL && dvz_instance_handle(instance) != VK_NULL_HANDLE)
            volkLoadInstance(dvz_instance_handle(instance));
    }
    if (fixture->runtime != NULL)
    {
        dvz_drp2_runtime_destroy(fixture->runtime);
        fixture->runtime = NULL;
    }
    if (fixture->gpu_ctx != NULL)
    {
        dvz_gpu_ctx_destroy(fixture->gpu_ctx);
        fixture->gpu_ctx = NULL;
    }
    dvz_free(fixture);
}



/**
 * Return the reset shared runtime for one frame-plan execution test.
 *
 * @param suite active test context
 * @param out_gpu_ctx optional output borrowed GPU context
 * @return borrowed runtime, or NULL when the fixture is unavailable
 */
static DvzDrp2Runtime*
_scene_frame_plan_fixture_runtime(TstContext* suite, DvzGpuCtx** out_gpu_ctx)
{
    ANN(suite);
    SceneFramePlanGpuFixture* fixture =
        (SceneFramePlanGpuFixture*)tst_context_fixture(suite, TST_SCENE_FRAME_PLAN_GPU_FIXTURE);
    if (fixture == NULL || !fixture->available)
    {
        const char* skip_reason = fixture != NULL && fixture->skip_reason != NULL ?
                                      fixture->skip_reason :
                                      "GPU fixture unavailable";
        tst_skip(suite, skip_reason);
        return NULL;
    }
    dvz_drp2_runtime_reset(fixture->runtime);
    if (out_gpu_ctx != NULL)
        *out_gpu_ctx = fixture->gpu_ctx;
    return fixture->runtime;
}
#endif



/**
 * Add one representative visual to a panel for DVZR portable-recording coverage.
 *
 * @param scene the scene
 * @param panel the panel
 * @param kind visual family to add
 * @return true on success
 */
static bool _add_dvzr_visual(DvzScene* scene, DvzPanel* panel, SceneDvzrVisualKind kind)
{
    ANN(scene);
    ANN(panel);

    if (kind == SCENE_DVZR_VISUAL_POINT)
    {
        DvzVisual* visual = dvz_point(scene, 0);
        ANN(visual);
        vec3 positions[3] = {
            {-0.5f, -0.5f, 0.0f},
            {0.5f, -0.5f, 0.0f},
            {0.0f, 0.5f, 0.0f},
        };
        DvzColor colors[3] = {
            {255, 0, 0, 255},
            {0, 255, 0, 255},
            {0, 0, 255, 255},
        };
        float sizes[3] = {8.0f, 9.0f, 10.0f};
        return dvz_visual_set_data(visual, "position", positions, 3) == 0 &&
               dvz_visual_set_data(visual, "color", colors, 3) == 0 &&
               dvz_visual_set_data(visual, "size", sizes, 3) == 0 &&
               dvz_panel_add_visual(panel, visual, NULL) == 0;
    }

    if (kind == SCENE_DVZR_VISUAL_PRIMITIVE)
    {
        DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
        ANN(visual);
        vec3 positions[3] = {
            {-0.6f, -0.5f, 0.0f},
            {0.6f, -0.5f, 0.0f},
            {0.0f, 0.6f, 0.0f},
        };
        DvzColor colors[3] = {
            {255, 64, 32, 255},
            {64, 255, 32, 255},
            {64, 128, 255, 255},
        };
        return dvz_visual_set_data(visual, "position", positions, 3) == 0 &&
               dvz_visual_set_data(visual, "color", colors, 3) == 0 &&
               dvz_panel_add_visual(panel, visual, NULL) == 0;
    }

    if (kind == SCENE_DVZR_VISUAL_MESH)
    {
        DvzVisual* visual = dvz_mesh(scene, 0);
        ANN(visual);
        vec3 positions[4] = {
            {-0.8f, -0.8f, 0.0f},
            {-0.8f, 0.8f, 0.0f},
            {0.8f, -0.8f, 0.0f},
            {0.8f, 0.8f, 0.0f},
        };
        vec3 normals[4] = {
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        };
        DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
        DvzSceneBuffer* index_buffer = dvz_scene_buffer(
            scene, &(DvzSceneBufferDesc){
                       .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                       .stride = sizeof(DvzIndex),
                   });
        ANN(index_buffer);
        return dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) &&
               dvz_visual_set_data(visual, "position", positions, 4) == 0 &&
               dvz_visual_set_data(visual, "normal", normals, 4) == 0 &&
               dvz_visual_set_buffer(visual, "index", index_buffer) &&
               dvz_panel_add_visual(panel, visual, NULL) == 0;
    }

    if (kind == SCENE_DVZR_VISUAL_IMAGE)
    {
        DvzVisual* visual = dvz_image(scene, 0);
        ANN(visual);
        vec3 positions[4] = {
            {-0.5f, -0.5f, 0.0f},
            {-0.5f, 0.5f, 0.0f},
            {0.5f, -0.5f, 0.0f},
            {0.5f, 0.5f, 0.0f},
        };
        vec2 texcoords[4] = {
            {0.0f, 0.0f},
            {0.0f, 1.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        };
        uint8_t pixels[4 * 4 * 4] = {0};
        for (uint32_t i = 0; i < 4 * 4; i++)
        {
            pixels[i * 4 + 0] = (uint8_t)(32u + i * 8u);
            pixels[i * 4 + 1] = (uint8_t)(255u - i * 8u);
            pixels[i * 4 + 2] = 128;
            pixels[i * 4 + 3] = 255;
        }
        return dvz_visual_set_data(visual, "position", positions, 4) == 0 &&
               dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0 &&
               dvz_visual_set_texture(visual, pixels, 4, 4) == 0 &&
               dvz_panel_add_visual(panel, visual, NULL) == 0;
    }

    return false;
}


#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
/**
 * Return the fixed DRP2 id used by the depth-peeling graph lowering test.
 *
 * @param resource_id the FramePlan graph resource id
 * @return the corresponding DRP2 object id, or 0 when unknown
 */
static uint64_t _depth_peel_default_resource_id(const char* resource_id)
{
    ANN(resource_id);

    if (strcmp(resource_id, "rt") == 0)
        return 50;
    if (strcmp(resource_id, "panel0.depth.opaque") == 0)
        return 51;
    if (strcmp(resource_id, "panel0.peel.front_accum") == 0)
        return 52;
    if (strcmp(resource_id, "panel0.peel.back_accum") == 0)
        return 53;
    if (strcmp(resource_id, "panel0.peel.depth_minmax_ping") == 0)
        return 54;
    if (strcmp(resource_id, "panel0.peel.depth_minmax_pong") == 0)
        return 55;
    return 0;
}



/**
 * Add a graph resource to runtime texture mapping to the depth-peeling test table.
 *
 * @param targets runtime target table
 * @param resource_id graph resource id
 * @param texture_id runtime texture id
 * @return true when the mapping was added
 */
static bool _depth_peel_runtime_targets_add(
    SceneDepthPeelRuntimeTargets* targets, const char* resource_id, uint64_t texture_id)
{
    ANN(targets);
    ANN(resource_id);

    if (texture_id == 0 || targets->count >= DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY)
        return false;

    for (uint32_t i = 0; i < targets->count; i++)
    {
        if (strcmp(targets->targets[i].resource_id, resource_id) == 0)
            return false;
    }

    SceneDepthPeelRuntimeTarget* target = &targets->targets[targets->count++];
    dvz_strlcpy(target->resource_id, resource_id, sizeof(target->resource_id));
    target->texture_id = texture_id;
    return true;
}



/**
 * Resolve a graph resource id to the runtime texture id used by the depth-peeling test.
 *
 * @param targets runtime target table
 * @param resource_id graph resource id
 * @return runtime texture id, or 0 when not mapped
 */
static uint64_t _depth_peel_runtime_targets_get(
    const SceneDepthPeelRuntimeTargets* targets, const char* resource_id)
{
    ANN(targets);
    ANN(resource_id);

    for (uint32_t i = 0; i < targets->count; i++)
    {
        if (strcmp(targets->targets[i].resource_id, resource_id) == 0)
            return targets->targets[i].texture_id;
    }
    return 0;
}



/**
 * Resolve a graph color attachment to the runtime texture id used by the test bridge.
 *
 * @param pass graph pass containing color attachments
 * @param index color attachment index
 * @param targets runtime target table
 * @return runtime texture id, or 0 when not resolved
 */
static uint64_t _depth_peel_color_attachment_texture_id(
    const DvzFrameGraphPass* pass, uint32_t index, const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(pass);
    ANN(targets);

    if (index >= pass->color_attachment_count)
        return 0;
    return _depth_peel_runtime_targets_get(
        targets, pass->color_attachments[index].resource_id);
}



/**
 * Resolve a graph sampled read to the runtime texture id used by the test bridge.
 *
 * @param pass graph pass containing sampled reads
 * @param index read index
 * @param targets runtime target table
 * @return runtime texture id, or 0 when not resolved
 */
static uint64_t _depth_peel_sampled_read_texture_id(
    const DvzFrameGraphPass* pass, uint32_t index, const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(pass);
    ANN(targets);

    if (index >= pass->read_count)
        return 0;
    return _depth_peel_runtime_targets_get(targets, pass->reads[index].resource_id);
}



/**
 * Resolve a graph depth attachment to the runtime texture id used by the test bridge.
 *
 * @param pass graph pass containing the depth attachment
 * @param targets runtime target table
 * @return runtime texture id, or 0 when not resolved
 */
static uint64_t _depth_peel_depth_attachment_texture_id(
    const DvzFrameGraphPass* pass, const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(pass);
    ANN(targets);

    if (!pass->has_depth_attachment)
        return 0;
    return _depth_peel_runtime_targets_get(targets, pass->depth_attachment.resource_id);
}



/**
 * Verify every graph resource has a runtime texture mapping.
 *
 * @param plan frame plan whose graph resources must be mapped
 * @param targets runtime target table
 * @return 0 on success
 */
static int _assert_depth_peel_runtime_targets(
    const DvzFramePlan* plan, const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(plan);
    ANN(targets);

    uint32_t resource_count = dvz_frame_plan_graph_resource_count(plan);
    AT(targets->count == resource_count);
    for (uint32_t i = 0; i < resource_count; i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        ANN(resource);
        AT(_depth_peel_runtime_targets_get(targets, resource->id) != 0);
    }
    return 0;
}



/**
 * Convert graph resource usage flags to the DRP2 texture usage bits needed by the test.
 *
 * @param usage graph resource usage flags
 * @return DRP2 texture usage flags
 */
static uint32_t _depth_peel_texture_usage(uint32_t usage)
{
    uint32_t out = 0;
    if ((usage & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0 ||
        (usage & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
    if ((usage & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    if ((usage & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    if ((usage & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    return out;
}



/**
 * Build the depth-peeling graph shape used by FramePlan graph and DRP2 lowering tests.
 *
 * @param out output frame plan owned by the caller
 * @return 0 on success
 */
static int _depth_peel_frame_plan_graph(DvzFramePlan** out)
{
    ANN(out);

    DvzFramePlan* plan = dvz_frame_plan("figure.emit.depth_peel", 20);
    ANN(plan);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.format = VK_FORMAT_R8G8B8A8_UNORM;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                     DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    AT(dvz_frame_plan_graph_resource(plan, &rt));

    DvzFrameGraphResource opaque_depth = {0};
    dvz_strlcpy(opaque_depth.id, "panel0.depth.opaque", sizeof(opaque_depth.id));
    opaque_depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    opaque_depth.format = VK_FORMAT_D32_SFLOAT;
    opaque_depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    opaque_depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    opaque_depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &opaque_depth));

    const char* color_ids[4] = {
        "panel0.peel.front_accum",
        "panel0.peel.back_accum",
        "panel0.peel.depth_minmax_ping",
        "panel0.peel.depth_minmax_pong",
    };
    for (uint32_t i = 0; i < 4; i++)
    {
        DvzFrameGraphResource resource = {0};
        dvz_strlcpy(resource.id, color_ids[i], sizeof(resource.id));
        resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        resource.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                               DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        AT(dvz_frame_plan_graph_resource(plan, &resource));
    }

    DvzFrameGraphAttachment rt_clear = {0};
    dvz_strlcpy(rt_clear.resource_id, "rt", sizeof(rt_clear.resource_id));
    rt_clear.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    rt_clear.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    rt_clear.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphAttachment depth_write = {0};
    dvz_strlcpy(depth_write.resource_id, "panel0.depth.opaque", sizeof(depth_write.resource_id));
    depth_write.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    depth_write.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    depth_write.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    depth_write.clear_depth = 1.0f;

    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, "panel0.opaque", sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, "panel.0", sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&opaque, &rt_clear));
    AT(dvz_frame_graph_pass_depth_attachment(&opaque, &depth_write));
    AT(dvz_frame_plan_graph_pass(plan, &opaque));

    DvzFrameGraphAttachment depth_read = {0};
    dvz_strlcpy(depth_read.resource_id, "panel0.depth.opaque", sizeof(depth_read.resource_id));
    depth_read.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    depth_read.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE;
    depth_read.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ;

    DvzFrameGraphPass init = {0};
    dvz_strlcpy(init.id, "panel0.peel.init", sizeof(init.id));
    dvz_strlcpy(init.panel_id, "panel.0", sizeof(init.panel_id));
    dvz_strlcpy(init.work_label, "depth_peel_init", sizeof(init.work_label));
    init.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    for (uint32_t i = 0; i < 3; i++)
    {
        DvzFrameGraphAttachment attachment = {0};
        dvz_strlcpy(attachment.resource_id, color_ids[i], sizeof(attachment.resource_id));
        attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
        attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
        attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
        AT(dvz_frame_graph_pass_color_attachment(&init, &attachment));
    }
    AT(dvz_frame_graph_pass_depth_attachment(&init, &depth_read));
    AT(dvz_frame_plan_graph_pass(plan, &init));

    for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
    {
        const bool even = (iter_idx % 2) == 0;
        const char* read_depth = even ? color_ids[2] : color_ids[3];
        const char* write_depth = even ? color_ids[3] : color_ids[2];

        DvzFrameGraphPass iter = {0};
        dvz_snprintf(iter.id, sizeof(iter.id), "panel0.peel.iter.%u", iter_idx);
        dvz_strlcpy(iter.panel_id, "panel.0", sizeof(iter.panel_id));
        dvz_strlcpy(iter.work_label, "depth_peel_iter", sizeof(iter.work_label));
        iter.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
        AT(dvz_frame_graph_pass_read(&iter, read_depth, DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
        for (uint32_t i = 0; i < 3; i++)
        {
            DvzFrameGraphAttachment attachment = {0};
            const char* resource_id = i == 0 ? color_ids[0] : i == 1 ? color_ids[1] : write_depth;
            dvz_strlcpy(attachment.resource_id, resource_id, sizeof(attachment.resource_id));
            attachment.load_op = i < 2 ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD :
                                         DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
            attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
            attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
            AT(dvz_frame_graph_pass_color_attachment(&iter, &attachment));
        }
        AT(dvz_frame_graph_pass_depth_attachment(&iter, &depth_read));
        AT(dvz_frame_plan_graph_pass(plan, &iter));
    }

    DvzFrameGraphAttachment rt_load = {0};
    dvz_strlcpy(rt_load.resource_id, "rt", sizeof(rt_load.resource_id));
    rt_load.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    rt_load.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    rt_load.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass composite = {0};
    dvz_strlcpy(composite.id, "panel0.peel.composite", sizeof(composite.id));
    dvz_strlcpy(composite.panel_id, "panel.0", sizeof(composite.panel_id));
    dvz_strlcpy(composite.work_label, "depth_peel_composite", sizeof(composite.work_label));
    composite.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_read(
        &composite, "panel0.peel.front_accum", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_read(
        &composite, "panel0.peel.back_accum", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_color_attachment(&composite, &rt_load));
    AT(dvz_frame_plan_graph_pass(plan, &composite));

    *out = plan;
    return 0;
}



/**
 * Append DRP2 texture creation commands for every resource declared by a graph.
 *
 * @param stream the command stream
 * @param plan the frame plan with graph resources
 * @param targets output runtime target table
 */
static int _depth_peel_emit_graph_resources(
    DvzDrp2CommandStream* stream, const DvzFramePlan* plan, SceneDepthPeelRuntimeTargets* targets)
{
    ANN(stream);
    ANN(plan);
    ANN(targets);

    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        ANN(resource);
        uint64_t id = _depth_peel_default_resource_id(resource->id);
        AT(id != 0);
        uint32_t usage = _depth_peel_texture_usage(resource->usage_flags);
        if (strcmp(resource->id, "rt") == 0)
            usage |= DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;

        AT(dvz_drp2_stream_create_texture_2d_format_usage(
            stream, id, 2, 2, resource->format, usage));
        AT(_depth_peel_runtime_targets_add(targets, resource->id, id));
    }
    AT(_assert_depth_peel_runtime_targets(plan, targets) == 0);
    return 0;
}



/**
 * Append the synthetic fullscreen shader and pipeline setup for graph lowering coverage.
 *
 * @param stream the command stream
 */
static int _depth_peel_emit_pipeline_setup(DvzDrp2CommandStream* stream)
{
    ANN(stream);

    AT(dvz_drp2_stream_create_sampler(stream, 2));

    DvzDrp2BindGroupLayoutEntry sampled_entries[3] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 3, 3, sampled_entries));

    const char* fullscreen_vs =
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}";
    AT(dvz_drp2_stream_create_shader_module_format(stream, 10, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 11, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;"
        "void main(){color=vec4(0.05,0.05,0.05,1.0);}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, true, VK_COMPARE_OP_LESS_OR_EQUAL));

    AT(dvz_drp2_stream_create_shader_module_format(stream, 20, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 21, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 front_accum;"
        "layout(location=1)out vec4 back_accum;layout(location=2)out vec4 depth_pair;"
        "void main(){front_accum=vec4(0.25,0,0,1);back_accum=vec4(0,0.25,0,1);"
        "depth_pair=vec4(gl_FragCoord.z,1.0-gl_FragCoord.z,0,1);}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 22, 20, 21, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 1, VK_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 2, VK_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, false, VK_COMPARE_OP_LESS_OR_EQUAL));
    AT(dvz_drp2_stream_pipeline_set_raster_state(
        stream, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE));

    AT(dvz_drp2_stream_create_shader_module_format(stream, 30, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 31, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 front_accum;"
        "layout(location=1)out vec4 back_accum;"
        "layout(location=2)out vec4 depth_pair;"
        "void main(){front_accum=vec4(0);back_accum=vec4(0,0.25,0,1);"
        "depth_pair=vec4(gl_FragCoord.z,1.0-gl_FragCoord.z,0,1);}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 32, 30, 31, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 1, VK_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 2, VK_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, false, VK_COMPARE_OP_LESS_OR_EQUAL));
    AT(dvz_drp2_stream_pipeline_set_raster_state(
        stream, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE));

    AT(dvz_drp2_stream_create_shader_module_format(stream, 40, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 41, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform texture2D front_accum;"
        "layout(set=0,binding=1)uniform texture2D back_accum;"
        "layout(set=0,binding=2)uniform sampler samp;"
        "layout(location=0)out vec4 color;"
        "void main(){ivec2 uv=ivec2(gl_FragCoord.xy);"
        "vec4 f=texelFetch(sampler2D(front_accum,samp),uv,0);"
        "vec4 b=texelFetch(sampler2D(back_accum,samp),uv,0);"
        "color=vec4(f.r,b.g,0.0,1.0);}"));
    AT(dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
        stream, 42, 40, 41, 0, 3));
    return 0;
}



/**
 * Append bind groups that sample graph ping and pong resources.
 *
 * @param stream the command stream
 * @param plan the frame plan whose reads define sampled resources
 * @param targets runtime target table
 */
static int _depth_peel_emit_bind_groups(
    DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(stream);
    ANN(plan);
    ANN(targets);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);

    const DvzFrameGraphPass* composite =
        dvz_frame_plan_graph_pass_get(plan, 2 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    ANN(composite);
    AT(composite->read_count == 2);
    const uint64_t composite_front = _depth_peel_sampled_read_texture_id(composite, 0, targets);
    const uint64_t composite_back = _depth_peel_sampled_read_texture_id(composite, 1, targets);
    AT(composite_front != 0);
    AT(composite_back != 0);

    DvzDrp2BindGroupEntry composite_entries[3] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = composite_front,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = composite_back,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = 2,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 61, 3, 3, composite_entries));
    return 0;
}



/**
 * Append graph-ordered render passes for the synthetic depth-peeling stream.
 *
 * @param stream the command stream
 * @param plan the frame plan whose graph defines pass order and attachments
 * @param targets runtime target table
 */
static int _depth_peel_emit_graph_passes(
    DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(stream);
    ANN(plan);
    ANN(targets);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);

    AT(dvz_drp2_stream_begin_command_encoder(stream, 80));

    const DvzFrameGraphPass* opaque = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(opaque);
    AT(strcmp(opaque->id, "panel0.opaque") == 0);
    uint64_t rt_id = _depth_peel_color_attachment_texture_id(opaque, 0, targets);
    uint64_t depth_id = _depth_peel_depth_attachment_texture_id(opaque, targets);
    AT(rt_id != 0);
    AT(depth_id != 0);
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 81, 80, rt_id, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, depth_id, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 81, 12));
    AT(dvz_drp2_stream_draw(stream, 81, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 81));

    const DvzFrameGraphPass* init = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(init);
    AT(strcmp(init->id, "panel0.peel.init") == 0);
    AT(init->color_attachment_count == 3);
    const uint64_t init_front = _depth_peel_color_attachment_texture_id(init, 0, targets);
    const uint64_t init_back = _depth_peel_color_attachment_texture_id(init, 1, targets);
    const uint64_t init_depth_pair = _depth_peel_color_attachment_texture_id(init, 2, targets);
    AT(init_front != 0);
    AT(init_back != 0);
    AT(init_depth_pair != 0);
    AT(dvz_drp2_stream_begin_render_pass_clear(
        stream, 82, 80, init_front, 0, 0, 0, 0));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
        stream, init_back, 0, 0, 0, 0, true));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
        stream, init_depth_pair, 0, 0, 0, 0, true));
    depth_id = _depth_peel_depth_attachment_texture_id(init, targets);
    AT(depth_id != 0);
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, depth_id, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_ops(
        stream, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_access(
        stream, DVZ_DRP2_ATTACHMENT_ACCESS_READ));
    AT(dvz_drp2_stream_set_pipeline(stream, 82, 22));
    AT(dvz_drp2_stream_draw(stream, 82, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 82));

    for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
    {
        const uint64_t pass_id = 83 + iter_idx;
        const DvzFrameGraphPass* iter = dvz_frame_plan_graph_pass_get(plan, 2 + iter_idx);
        ANN(iter);
        char iter_id[64];
        dvz_snprintf(iter_id, sizeof(iter_id), "panel0.peel.iter.%u", iter_idx);
        AT(strcmp(iter->id, iter_id) == 0);
        AT(iter->color_attachment_count == 3);
        AT(iter->read_count == 1);
        const uint64_t iter_front = _depth_peel_color_attachment_texture_id(iter, 0, targets);
        const uint64_t iter_back = _depth_peel_color_attachment_texture_id(iter, 1, targets);
        const uint64_t iter_depth_pair = _depth_peel_color_attachment_texture_id(iter, 2, targets);
        AT(iter_front != 0);
        AT(iter_back != 0);
        AT(iter_depth_pair != 0);
        AT(dvz_drp2_stream_begin_render_pass_clear(
            stream, pass_id, 80, iter_front, 0, 0, 0, 0));
        AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
            stream, iter_back, 0, 0, 0, 0, true));
        AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
            stream, iter_depth_pair, 1, 0, 0, 0, true));
        depth_id = _depth_peel_depth_attachment_texture_id(iter, targets);
        AT(depth_id != 0);
        AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, depth_id, 1.0f));
        AT(dvz_drp2_stream_begin_render_pass_set_depth_ops(
            stream, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE));
        AT(dvz_drp2_stream_begin_render_pass_set_depth_access(
            stream, DVZ_DRP2_ATTACHMENT_ACCESS_READ));
        AT(dvz_drp2_stream_set_pipeline(stream, pass_id, 32));
        AT(dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0));
        AT(dvz_drp2_stream_end_render_pass(stream, pass_id));
    }

    const DvzFrameGraphPass* composite =
        dvz_frame_plan_graph_pass_get(plan, 2 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    ANN(composite);
    AT(strcmp(composite->id, "panel0.peel.composite") == 0);
    rt_id = _depth_peel_color_attachment_texture_id(composite, 0, targets);
    AT(rt_id != 0);
    const uint64_t composite_pass_id = 83 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS;
    AT(dvz_drp2_stream_begin_render_pass_clear(
        stream, composite_pass_id, 80, rt_id, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
        stream, 0, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_STORE));
    AT(dvz_drp2_stream_set_pipeline(stream, composite_pass_id, 42));
    AT(dvz_drp2_stream_set_bind_group(stream, composite_pass_id, 0, 61));
    AT(dvz_drp2_stream_draw(stream, composite_pass_id, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, composite_pass_id));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 80, rt_id, 70, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 80, 90));
    AT(dvz_drp2_stream_queue_submit(stream, 90, 91));
    return 0;
}



/**
 * Lower the focused depth-peeling graph descriptor into a DRP2 command stream.
 *
 * @param plan the graph-only frame plan
 * @param out_stream output command stream owned by the caller
 * @param out_targets optional output runtime target table
 * @return 0 on success
 */
static int _depth_peel_graph_to_drp2(
    const DvzFramePlan* plan, DvzDrp2CommandStream** out_stream,
    SceneDepthPeelRuntimeTargets* out_targets)
{
    ANN(plan);
    ANN(out_stream);

    SceneDepthPeelRuntimeTargets targets = {0};

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "scene-graph-test"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));

    AT(_depth_peel_emit_pipeline_setup(stream) == 0);
    AT(_depth_peel_emit_graph_resources(stream, plan, &targets) == 0);
    AT(dvz_drp2_stream_create_buffer(
        stream, 70, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(_depth_peel_emit_bind_groups(stream, plan, &targets) == 0);
    AT(_depth_peel_emit_graph_passes(stream, plan, &targets) == 0);
    if (out_targets != NULL)
        *out_targets = targets;
    *out_stream = stream;
    return 0;
}



/**
 * Verify a begin-render-pass command matches graph color attachments.
 *
 * @param cmd stream command to inspect
 * @param pass graph pass defining color attachments
 * @param targets runtime target table
 * @return 0 on success
 */
static int _assert_depth_peel_pass_color_targets(
    const DvzDrp2Command* cmd, const DvzFrameGraphPass* pass,
    const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(cmd);
    ANN(pass);
    ANN(targets);
    AT(cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    AT(cmd->u.begin_render_pass.color_attachment_count == pass->color_attachment_count);
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        uint64_t id = _depth_peel_color_attachment_texture_id(pass, i, targets);
        AT(id != 0);
        AT(cmd->u.begin_render_pass.color_attachments[i].texture_id == id);
    }
    return 0;
}



/**
 * Verify a bind-group command samples graph pass reads.
 *
 * @param cmd stream command to inspect
 * @param bind_group_id expected bind group id
 * @param pass graph pass defining sampled reads
 * @param targets runtime target table
 * @return 0 on success
 */
static int _assert_depth_peel_bind_group_reads(
    const DvzDrp2Command* cmd, uint64_t bind_group_id, const DvzFrameGraphPass* pass,
    const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(cmd);
    ANN(pass);
    ANN(targets);
    AT(cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    AT(cmd->u.create_bind_group.id == bind_group_id);
    AT(pass->read_count == 2);
    AT(cmd->u.create_bind_group.entry_count == 3);
    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        uint64_t id = _depth_peel_sampled_read_texture_id(pass, i, targets);
        AT(id != 0);
        AT(cmd->u.create_bind_group.entries[i].resource_id == id);
    }
    return 0;
}



/**
 * Verify the lowered stream preserves graph resource ids in passes and bind groups.
 *
 * @param stream the stream to inspect
 * @param plan graph plan used for lowering
 * @param targets runtime target table
 * @return 0 on success
 */
static int _assert_depth_peel_graph_stream_shape(
    const DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const SceneDepthPeelRuntimeTargets* targets)
{
    ANN(stream);
    ANN(plan);
    ANN(targets);
    AT(_assert_depth_peel_runtime_targets(plan, targets) == 0);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);

    uint32_t named_depth_passes = 0;
    uint32_t three_color_passes = 0;
    uint32_t raster_pipelines = 0;
    bool sampled_accum = false;
    bool init_attachments = false;
    uint32_t iter_attachments = 0;
    bool composite_attachment = false;

    const DvzFrameGraphPass* opaque = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* init = dvz_frame_plan_graph_pass_get(plan, 1);
    const DvzFrameGraphPass* composite =
        dvz_frame_plan_graph_pass_get(plan, 2 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    ANN(opaque);
    ANN(init);
    ANN(composite);
    uint64_t depth_id = _depth_peel_depth_attachment_texture_id(opaque, targets);
    AT(depth_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            if (cmd->u.begin_render_pass.depth_texture_id == depth_id)
                named_depth_passes++;
            if (cmd->u.begin_render_pass.color_attachment_count == 3)
                three_color_passes++;
            if (cmd->u.begin_render_pass.id == 82)
            {
                AT(_assert_depth_peel_pass_color_targets(cmd, init, targets) == 0);
                AT(cmd->u.begin_render_pass.depth_texture_id == depth_id);
                init_attachments = true;
            }
            if (cmd->u.begin_render_pass.id == 83)
            {
                const DvzFrameGraphPass* iter = dvz_frame_plan_graph_pass_get(plan, 2);
                ANN(iter);
                AT(_assert_depth_peel_pass_color_targets(cmd, iter, targets) == 0);
                AT(cmd->u.begin_render_pass.depth_texture_id == depth_id);
                iter_attachments++;
            }
            if (cmd->u.begin_render_pass.id > 83 &&
                cmd->u.begin_render_pass.id < 83 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS)
            {
                uint32_t iter_idx = (uint32_t)(cmd->u.begin_render_pass.id - 83);
                const DvzFrameGraphPass* iter =
                    dvz_frame_plan_graph_pass_get(plan, 2 + iter_idx);
                ANN(iter);
                AT(_assert_depth_peel_pass_color_targets(cmd, iter, targets) == 0);
                AT(cmd->u.begin_render_pass.depth_texture_id == depth_id);
                iter_attachments++;
            }
            if (cmd->u.begin_render_pass.id == 83 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS)
            {
                AT(_assert_depth_peel_pass_color_targets(cmd, composite, targets) == 0);
                composite_attachment = true;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            if (cmd->u.create_render_pipeline.has_raster_state)
                raster_pipelines++;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const DvzDrp2BindGroupEntry* entries = cmd->u.create_bind_group.entries;
            if (cmd->u.create_bind_group.id == 61)
            {
                AT(entries[2].resource_id == 2);
                AT(_assert_depth_peel_bind_group_reads(cmd, 61, composite, targets) == 0);
                sampled_accum = true;
            }
        }
    }

    AT(named_depth_passes == 2 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    AT(three_color_passes == 1 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    AT(raster_pipelines == 2);
    AT(sampled_accum);
    AT(init_attachments);
    AT(iter_attachments == DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    AT(composite_attachment);
    return 0;
}


#endif



/**
 * Verify one scene visual family records to portable DVZR JSONL and replays semantically.
 *
 * @param kind visual family to test
 * @param suffix recording path suffix
 * @return 0 on success
 */
static int _scene_visual_records_portable_dvzr(SceneDvzrVisualKind kind, const char* suffix)
{
    ANN(suffix);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    AT(_add_dvzr_visual(scene, panel, kind));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream) > 0);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    DvzDrp2RecordingInfo info = {
        .width = 64,
        .height = 64,
        .duration_s = 0.0,
        .t_present = 0.0,
        .backend_hint = "semantic",
    };
    char path[256] = {0};
    dvz_snprintf(path, sizeof(path), "/tmp/dvz_scene_%s_emit_portable.dvzr", suffix);
    AT(dvz_drp2_recording_write_stream(path, stream, &info));

    char stream_path[256] = {0};
    dvz_snprintf(stream_path, sizeof(stream_path), "%s/stream.jsonl", path);
    FILE* stream_file = fopen(stream_path, "rb");
    ANN(stream_file);
    char jsonl[131072] = {0};
    size_t size = fread(jsonl, 1, sizeof(jsonl) - 1, stream_file);
    fclose(stream_file);
    AT(size > 0);
    AT(strstr(jsonl, ".cmd") == NULL);
    AT(strstr(jsonl, "\"op\":\"CreateShaderModule\"") != NULL);
    AT(strstr(jsonl, "\"op\":\"CreateBindGroupLayout\"") != NULL);
    AT(strstr(jsonl, "\"op\":\"CreateBindGroup\"") != NULL);
    AT(strstr(jsonl, "\"op\":\"CreateRenderPipeline\"") != NULL);
    AT(strstr(jsonl, "\"op\":\"QueueSubmit\"") != NULL);

    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    ANN(recording);
    AT(dvz_drp2_recording_frame_count(recording) == 1);
    const DvzDrp2CommandStream* loaded = dvz_drp2_recording_stream(recording);
    ANN(loaded);
    AT(dvz_drp2_stream_count(loaded) == dvz_drp2_stream_count(stream));

    runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    result = dvz_drp2_recording_playback(recording, runtime, false);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_recording_close(recording);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_frame_plan_emit_drp2_static_render(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert", 10);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(_frame_plan_render_fixture_visual(plan, "visual.point.0", "buf.point.position"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 16);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 0)) == DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 6)) ==
       DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 15)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_static_render_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateRenderPipeline\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateTexture\"") != NULL);
    AT(strstr(json, "\"cmd\": \"Draw\"") != NULL);
    AT(strstr(json, "\"command_buffer_ids\": [4]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_static_render_from_c",
           "spec/drp2/fixtures/positive/scene_static_render_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_static_render_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.glsl", 15);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(_frame_plan_render_fixture_visual(plan, "visual.point.0", "buf.point.position"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 16);

    char* json = dvz_drp2_stream_json(stream, "scene_static_render_glsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"glsl\"") != NULL);
    AT(strstr(json, "#version 450\\nlayout(location=0)in vec3 pos;") != NULL);
    AT(strstr(json, "\"format\": \"wgsl\"") == NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


static int test_frame_plan_emitter_rejects_untyped_visual_metadata(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.untyped_metadata", 16);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &cfg);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(dvz_diagnostic_report_get(&report, 0), "render visual missing typed metadata") == 0);

    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_rejects_unsupported_shader_format(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.unsupported_shader", 17);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(_frame_plan_render_fixture_visual(plan, "visual.point.0", "buf.point.position"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = false;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &cfg);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(dvz_diagnostic_report_get(&report, 0), "unsupported shader format") == 0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_rejects_small_caps(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.small_caps", 18);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(_frame_plan_render_fixture_visual(plan, "visual.point.0", "buf.point.position"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    caps.max_buffer_size = 8;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(dvz_diagnostic_report_get(&report, 0), "upload buffer exceeds max_buffer_size") == 0);

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_texture_dimension_2d = 3;
    stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_texture_dimension_2d is too small for fixture render target") == 0);

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_vertex_buffers = 0;
    stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_vertex_buffers is too small for fixture render pipeline") == 0);

    DvzFramePlan* texture_plan = dvz_frame_plan("figure.convert.small_bind_group_caps", 19);
    ANN(texture_plan);
    AT(dvz_frame_plan_upload(texture_plan, "tex.image.rgba", 0, 16, "image.rgba"));
    AT(dvz_frame_plan_render(texture_plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(texture_plan, "visual.image.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(texture_plan));

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_bind_groups = 0;
    stream = dvz_frame_plan_emit_drp2(texture_plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_bind_groups is too small for fixture bind groups") == 0);

    dvz_frame_plan_destroy(texture_plan);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_static_render_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_frame_plan_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.glsl.execute", 16);
    ANN(plan);
    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(_frame_plan_render_fixture_visual(plan, "visual.point.0", "buf.point.position"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &emit_cfg);
    ANN(stream);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_readback_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_frame_plan_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzFramePlan* plan = dvz_frame_plan("figure.readback.glsl.execute", 20);
    ANN(plan);
    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.picking", true));
    AT(_frame_plan_render_fixture_visual(plan, "visual.pickable.0", "buf.point.position"));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(plan, "buf.pick.readback", "request.pick.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &emit_cfg);
    ANN(stream);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 12, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emitter_runtime_two_frames_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_frame_plan_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.point.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(_frame_plan_render_fixture_visual(frame0, "visual.pickable.0", "buf.point.position"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.point.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(_frame_plan_render_fixture_visual(frame1, "visual.pickable.0", "buf.point.position"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_dynamic_two_frames_glsl_executes(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_frame_plan_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.dynamic.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.dynamic.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.color", 0, 16, "point.color.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.dynamic.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.color", 0, 16, "point.color.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.dynamic.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_texture_two_frames_glsl_executes(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_frame_plan_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.texture.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.texture.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "tex.image.rgba", 0, 16, "image.rgba.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.image.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "tex.image.rgba", 0, 16, "image.rgba.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.image.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_frame_plan_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.compute.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.compute.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.compute.input", 0, 36, "compute.input.0"));
    AT(dvz_frame_plan_compute(frame0, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame0, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame0, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.compute.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.compute.input", 0, 36, "compute.input.1"));
    AT(dvz_frame_plan_compute(frame1, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame1, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame1, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.compute.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_frame_plan_emit_drp2_depth_peeling_graph_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzFramePlan* plan = NULL;
    AT(_depth_peel_frame_plan_graph(&plan) == 0);
    ANN(plan);

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzDrp2CommandStream* stream = NULL;
    SceneDepthPeelRuntimeTargets targets = {0};
    AT(_depth_peel_graph_to_drp2(plan, &stream, &targets) == 0);
    ANN(stream);
    AT(_assert_depth_peel_graph_stream_shape(stream, plan, &targets) == 0);

    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(validation.code == DVZ_DRP2_VALIDATION_OK);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceFeatures features10 = {0};
    features10.independentBlend = true;
    dvz_gpu_ctx_config_features10(&gpu_cfg, &features10);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene depth-peeling graph DRP2 test skipped because GPU context creation failed");
        dvz_drp2_stream_destroy(stream);
        dvz_frame_plan_destroy(plan);
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t resolved[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 70, 0, 4, resolved));
    AT(resolved[0] > 0 || resolved[1] > 0 || resolved[2] > 0);
    AT(resolved[3] == 255);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}
#endif


int test_scene_drp2_offscreen_canvas_frame(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene offscreen canvas test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzWindowHost* host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "scene-drp2-offscreen-canvas";
    window_cfg.width = 64;
    window_cfg.height = 64;
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        log_warn("scene offscreen canvas test skipped because headless window creation failed");
        dvz_window_host_destroy(host);
        dvz_gpu_ctx_destroy(ctx);
        tst_skip(suite, "headless window creation failed");
        return 0;
    }

    DvzCanvasConfig canvas_cfg = dvz_canvas_default_config();
    canvas_cfg.window = window;
    canvas_cfg.device = dvz_gpu_ctx_device(ctx);
    canvas_cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    canvas_cfg.timing_history = 2;
    DvzCanvas* canvas = dvz_canvas_create(&canvas_cfg);
    ANN(canvas);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    SceneCanvasDrawState state = {0};
    state.emitter = emitter;
    state.runtime = runtime;
    state.emit_cfg = dvz_frame_plan_emit_config();
    state.emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    state.emit_cfg.external_color_target = true;
    state.emit_cfg.color_target_id = 1;
    state.emit_cfg.fullscreen_triangle = true;
    dvz_capability_snapshot_default(&state.caps);

    dvz_canvas_set_draw_callback(canvas, _scene_canvas_drp2_draw, &state);
    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    AT(state.callback_count == 1);
    AT(state.attach_ok);
    AT(state.emit_ok);
    AT(state.direct_target_ok);
    AT(state.execute_ok);
    AT(dvz_canvas_submit(canvas) == 0);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);
    uint32_t bright_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] > 200 && pixel[2] > 200 && pixel[3] > 200)
        {
            bright_count++;
        }
    }
    AT(bright_count > (width * height) / 2);
    dvz_free(rgba);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_frame_plan_emitter_destroy(emitter);
    dvz_drp2_runtime_destroy(runtime);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_frame_plan_emit_drp2_readback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.readback.convert", 11);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(plan, "visual.pickable.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(plan));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(plan, "buf.pick.readback", "request.pick.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 15)) ==
       DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_readback_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CopyTextureToBuffer\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\", \"MAP_READ\"]") != NULL);
    AT(strstr(json, "\"readbacks\": [ { \"buffer_id\": 12, \"offset\": 0, \"size\": 4 } ]") !=
       NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_readback_from_c",
           "spec/drp2/fixtures/positive/scene_readback_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_dynamic_uploads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.dynamic.convert", 12);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.dynamic.position", 0, 16, "point.position.update"));
    AT(dvz_frame_plan_upload(plan, "buf.dynamic.color", 0, 16, "point.color.update"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.dynamic.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(plan));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 3)) == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 5)) == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 13)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_dynamic_uploads_from_c");
    ANN(json);
    AT(strstr(json, "\"id\": 20") != NULL);
    AT(strstr(json, "\"id\": 21") != NULL);
    AT(strstr(json, "\"buffer_id\": 20") != NULL);
    AT(strstr(json, "\"buffer_id\": 21") != NULL);
    AT(strstr(json, "\"cmd\": \"Draw\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_dynamic_uploads_from_c",
           "spec/drp2/fixtures/positive/scene_dynamic_uploads_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_texture_sampling(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.texture.convert", 13);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "tex.image.rgba", 0, 16, "image.rgba"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.image.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(plan));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 19);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 3)) == DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_SAMPLER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 5)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 9)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 14)) == DVZ_DRP2_COMMAND_SET_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 18)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_texture_sampling_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateSampler\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBindGroupLayout\"") != NULL);
    AT(strstr(json, "\"cmd\": \"SetBindGroup\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\", \"TEXTURE_BINDING\"]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_texture_sampling_from_c",
           "spec/drp2/fixtures/positive/scene_texture_sampling_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_compute_assisted(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.compute.convert", 14);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.compute.input", 0, 36, "compute.input"));
    AT(dvz_frame_plan_compute(plan, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(plan, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(plan, "buf.compute.output"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.compute.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(plan));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 26);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 7)) ==
       DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 14)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 24)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 25)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_compute_assisted_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"BeginComputePass\"") != NULL);
    AT(strstr(json, "\"cmd\": \"DispatchWorkgroups\"") != NULL);
    AT(strstr(json, "\"usage\": [\"VERTEX\", \"STORAGE\"]") != NULL);
    AT(strstr(json, "\"binding_type\": \"storage_buffer\"") != NULL);
    AT(strstr(json, "\"binding\": 0, \"binding_type\": \"storage_buffer\", \"visibility\": "
                    "[\"COMPUTE\"], \"access\": \"read\"") != NULL);
    AT(strstr(json, "\"binding\": 1, \"binding_type\": \"storage_buffer\", \"visibility\": "
                    "[\"COMPUTE\"], \"access\": \"read_write\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_compute_assisted_from_c",
           "spec/drp2/fixtures/positive/scene_compute_assisted_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emitter_runtime_two_frames(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.point.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.pickable.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.point.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.pickable.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 0)) ==
       DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 16)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 10);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 8)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 9)) ==
       DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_dynamic_two_frames(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.dynamic", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.dynamic", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.color", 0, 16, "point.color.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.dynamic.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.color", 0, 16, "point.color.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.dynamic.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 21);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 14)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 12);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 5)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 6)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_dynamic_grow_buffer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.grow", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.grow", 1);
    ANN(frame0);
    ANN(frame1);

    AT(dvz_frame_plan_upload(frame0, "buf.grow.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame0, "visual.grow.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));

    AT(dvz_frame_plan_upload(frame1, "buf.grow.position", 0, 256, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame1, "visual.grow.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


/**
 * Ensure runtime texture uploads recreate the logical texture when the full extent changes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_emitter_runtime_texture_extent_changes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.texture.resize", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.texture.resize", 1);
    DvzFramePlan* frame2 = dvz_frame_plan("figure.runtime.texture.resize", 2);
    DvzFramePlan* frame3 = dvz_frame_plan("figure.runtime.texture.resize", 3);
    ANN(frame0);
    ANN(frame1);
    ANN(frame2);
    ANN(frame3);

    AT(dvz_frame_plan_upload(frame0, "tex.resize.rgba", 0, 16, "image.rgba.0"));
    AT(dvz_frame_plan_upload_set_texture_extent(frame0, 2, 2));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.image.resize"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));

    AT(dvz_frame_plan_upload(frame1, "tex.resize.rgba", 0, 16, "image.rgba.1"));
    AT(dvz_frame_plan_upload_set_texture_extent(frame1, 2, 2));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.image.resize"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));

    AT(dvz_frame_plan_upload(frame2, "tex.resize.rgba", 0, 64, "image.rgba.2"));
    AT(dvz_frame_plan_upload_set_texture_extent(frame2, 4, 4));
    AT(dvz_frame_plan_render(frame2, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame2, "visual.image.resize"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame2));

    AT(dvz_frame_plan_upload(frame3, "tex.partial.rgba", 0, 4, "image.rgba.3"));
    AT(dvz_frame_plan_upload_set_texture_extent(frame3, 1, 1));
    AT(dvz_frame_plan_upload_set_texture_allocation_extent(frame3, 4, 4));
    AT(dvz_frame_plan_upload_set_texture_region(frame3, 3, 3));
    AT(dvz_frame_plan_render(frame3, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame3, "visual.image.partial"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame3));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex0 = 0;
    bool created_tex0 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            tex0 = cmd->u.write_texture.texture_id;
    }
    AT(tex0 != 0);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex0)
        {
            created_tex0 = true;
            AT(cmd->u.create_texture.width == 2);
            AT(cmd->u.create_texture.height == 2);
        }
    }
    AT(created_tex0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool recreated_tex0 = false;
    bool wrote_tex0 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex0)
            recreated_tex0 = true;
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == tex0)
        {
            wrote_tex0 = true;
        }
    }
    AT(!recreated_tex0);
    AT(wrote_tex0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame2, &caps, &report, &emit_cfg);
    ANN(stream2);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex2 = 0;
    bool created_tex2 = false;
    bool wrote_tex2 = false;
    bool rebound_tex2 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            tex2 = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex2)
        {
            created_tex2 = true;
            AT(cmd->u.create_texture.width == 4);
            AT(cmd->u.create_texture.height == 4);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == tex2)
        {
            wrote_tex2 = true;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            for (uint32_t j = 0; j < cmd->u.create_bind_group.entry_count; j++)
            {
                if (cmd->u.create_bind_group.entries[j].resource_id == tex2)
                    rebound_tex2 = true;
            }
        }
    }
    AT(tex2 != 0);
    AT(tex2 != tex0);
    AT(created_tex2);
    AT(wrote_tex2);
    AT(rebound_tex2);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame3, &caps, &report, &emit_cfg);
    ANN(stream3);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex3 = 0;
    bool created_tex3 = false;
    bool wrote_tex3_region = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream3); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream3, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            tex3 = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream3); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream3, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex3)
        {
            created_tex3 = true;
            AT(cmd->u.create_texture.width == 4);
            AT(cmd->u.create_texture.height == 4);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == tex3 &&
            cmd->u.write_texture.width == 1 &&
            cmd->u.write_texture.height == 1 &&
            cmd->u.write_texture.origin_x == 3 &&
            cmd->u.write_texture.origin_y == 3)
        {
            wrote_tex3_region = true;
        }
    }
    AT(tex3 != 0);
    AT(created_tex3);
    AT(wrote_tex3_region);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream2);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream3);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream3);
    dvz_drp2_stream_destroy(stream2);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame3);
    dvz_frame_plan_destroy(frame2);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


/**
 * Ensure persistent emitter object ids can grow beyond the initial resource-map capacity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_emitter_runtime_object_map_grows(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    uint32_t count = DRP2_MAX_FIXTURE_RESOURCES + 17;
    for (uint32_t i = 0; i < count; i++)
    {
        char key[DVZ_SCENE_LABEL_SIZE] = {0};
        dvz_snprintf(key, sizeof(key), "grow.object.%u", i);

        bool is_new = false;
        uint64_t id = _obj_id(emitter, key, &is_new);
        AT(id != 0);
        AT(is_new);
        AT(dvz_frame_plan_emitter_object_id(emitter, key) == id);
    }

    bool is_new = true;
    uint64_t id = _obj_id(emitter, "grow.object.3", &is_new);
    AT(id != 0);
    AT(!is_new);
    AT(dvz_frame_plan_emitter_object_id(emitter, "grow.object.3") == id);

    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_texture_two_frames(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.texture", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.texture", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "tex.image.rgba", 0, 16, "image.rgba.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.image.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "tex.image.rgba", 0, 16, "image.rgba.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.image.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 21);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_SAMPLER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 5)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 9)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 10);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 4)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_compute_two_frames(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.compute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.compute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.compute.input", 0, 36, "compute.input.0"));
    AT(dvz_frame_plan_compute(frame0, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame0, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame0, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.compute.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame0));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.compute.input", 0, 36, "compute.input.1"));
    AT(dvz_frame_plan_compute(frame1, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame1, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame1, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.compute.0"));
    AT(dvz_frame_plan_render_allow_untyped_visual_compat(frame1));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 28);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 7)) ==
       DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 18)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 15);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 4)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 5)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


static int test_frame_plan_emit_scene_core_visuals_record_portable_dvzr(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    AT(_scene_visual_records_portable_dvzr(SCENE_DVZR_VISUAL_POINT, "point") == 0);
    AT(_scene_visual_records_portable_dvzr(SCENE_DVZR_VISUAL_PRIMITIVE, "primitive") == 0);
    AT(_scene_visual_records_portable_dvzr(SCENE_DVZR_VISUAL_MESH, "mesh") == 0);
    AT(_scene_visual_records_portable_dvzr(SCENE_DVZR_VISUAL_IMAGE, "image") == 0);
    return 0;
}


/**
 * Register scene frameplan emission tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_frame_plan_emit(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("frame-plan-emit");

    TST_CASE(test_frame_plan_emit_drp2_static_render);
    TST_CASE(test_frame_plan_emit_drp2_static_render_glsl);
    TST_CASE(test_frame_plan_emitter_rejects_untyped_visual_metadata);
    TST_CASE(test_frame_plan_emit_drp2_rejects_unsupported_shader_format);
    TST_CASE(test_frame_plan_emit_drp2_rejects_small_caps);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    tst_suite_register_fixture(
        suite, TST_SCENE_FRAME_PLAN_GPU_FIXTURE, TST_FIXTURE_SCOPE_PROCESS,
        _scene_frame_plan_gpu_fixture_create, _scene_frame_plan_gpu_fixture_destroy);
    TST_SCENE_SHARED_GPU_CASE(test_frame_plan_emit_drp2_static_render_glsl_executes);
    TST_SCENE_SHARED_GPU_CASE(test_frame_plan_emit_drp2_readback_glsl_executes);
    TST_SCENE_SHARED_GPU_CASE(test_frame_plan_emitter_runtime_two_frames_glsl_executes);
    TST_SCENE_SHARED_GPU_CASE(test_frame_plan_emitter_runtime_dynamic_two_frames_glsl_executes);
    TST_SCENE_SHARED_GPU_CASE(test_frame_plan_emitter_runtime_texture_two_frames_glsl_executes);
    TST_SCENE_SHARED_GPU_CASE(test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes);
    TST_SCENE_GPU_CASE(test_frame_plan_emit_drp2_depth_peeling_graph_executes);
    TST_SCENE_GPU_CASE(test_scene_drp2_offscreen_canvas_frame);
#endif
    TST_CASE(test_frame_plan_emit_drp2_readback);
    TST_CASE(test_frame_plan_emit_drp2_dynamic_uploads);
    TST_CASE(test_frame_plan_emit_drp2_texture_sampling);
    TST_CASE(test_frame_plan_emit_drp2_compute_assisted);
    TST_CASE(test_frame_plan_emitter_runtime_two_frames);
    TST_CASE(test_frame_plan_emitter_runtime_dynamic_two_frames);
    TST_CASE(test_frame_plan_emitter_runtime_dynamic_grow_buffer);
    TST_CASE(test_frame_plan_emitter_runtime_texture_extent_changes);
    TST_CASE(test_frame_plan_emitter_runtime_object_map_grows);
    TST_CASE(test_frame_plan_emitter_runtime_texture_two_frames);
    TST_CASE(test_frame_plan_emitter_runtime_compute_two_frames);
    TST_CASE(test_frame_plan_emit_scene_core_visuals_record_portable_dvzr);

    return 0;
}
