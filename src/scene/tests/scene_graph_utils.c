/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph test helpers                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_graph_utils.h"
#include "frame_plan/internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Resolve the typed provider represented by one physical graph pass.
 *
 * @param plan source frame plan
 * @param pass physical graph pass
 * @return provider key, or none when the pass is not composition-backed
 */
DvzSceneWorkProviderKey _scene_test_graph_pass_provider(
    const DvzFramePlan* plan, const DvzFrameGraphPass* pass)
{
    ANN(plan);
    if (pass == NULL || !pass->has_composition_pass)
        return DVZ_SCENE_WORK_PROVIDER_NONE;
    const DvzPanelCompositionSnapshot* snapshot =
        _frame_plan_composition_get(plan, pass->panel_id);
    if (snapshot == NULL)
        return DVZ_SCENE_WORK_PROVIDER_NONE;
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        if (snapshot->passes[i].id.value == pass->composition_pass_id.value)
            return snapshot->passes[i].provider;
    }
    return DVZ_SCENE_WORK_PROVIDER_NONE;
}



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
typedef struct
{
    DvzGpuCtx* gpu_ctx;
    DvzDrp2Runtime* runtime;
    bool available;
    const char* skip_reason;
} SceneGraphGpuFixture;
#endif



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
/**
 * Return the GPU context configuration for shared scene-graph DRP2 execution tests.
 *
 * @return GPU context configuration with required Vulkan 1.3 features
 */
static DvzGpuCtxConfig _scene_graph_gpu_ctx_config(const TstSuite* suite)
{
    DvzGpuCtxConfig gpu_cfg = dvz_testing_suite_gpu_ctx_config(suite);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    return gpu_cfg;
}



/**
 * Create a process-scoped GPU/runtime fixture for scene-graph DRP2 execution tests.
 *
 * @param suite active test suite
 * @param worker_index scheduler worker index
 * @return fixture state
 */
static void* _scene_graph_gpu_fixture_create(TstSuite* suite, uint32_t worker_index)
{
    (void)worker_index;

    SceneGraphGpuFixture* fixture =
        (SceneGraphGpuFixture*)dvz_calloc(1, sizeof(SceneGraphGpuFixture));
    ANN(fixture);

    if (!_scene_vklite_runtime_available())
    {
        fixture->skip_reason = "Vulkan instance creation failed";
        return fixture;
    }

    DvzGpuCtxConfig gpu_cfg = _scene_graph_gpu_ctx_config(suite);
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
 * Destroy the process-scoped scene-graph DRP2 GPU fixture.
 *
 * @param fixture_ptr fixture state
 */
static void _scene_graph_gpu_fixture_destroy(void* fixture_ptr)
{
    SceneGraphGpuFixture* fixture = (SceneGraphGpuFixture*)fixture_ptr;
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
 * Return the reset shared runtime for one scene-graph execution test.
 *
 * @param suite active test context
 * @param out_gpu_ctx optional output borrowed GPU context
 * @return borrowed runtime, or NULL when the fixture is unavailable
 */
DvzDrp2Runtime* _scene_graph_fixture_runtime(TstContext* suite, DvzGpuCtx** out_gpu_ctx)
{
    ANN(suite);
    SceneGraphGpuFixture* fixture =
        (SceneGraphGpuFixture*)tst_context_fixture(suite, TST_SCENE_GRAPH_GPU_FIXTURE);
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
 * Return whether a command creates the scene common bind-group layout.
 *
 * @param cmd the command to inspect
 * @return whether the command creates the common MVP/viewport layout
 */
static bool _is_scene_common_bind_group_layout(const DvzDrp2Command* cmd)
{
    ANN(cmd);
    if (cmd->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
        return false;
    if (cmd->u.create_bind_group_layout.entry_count != 2)
        return false;
    return cmd->u.create_bind_group_layout.entries[0].binding == 0 &&
           cmd->u.create_bind_group_layout.entries[1].binding == 1 &&
           cmd->u.create_bind_group_layout.entries[0].binding_type ==
               DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
           cmd->u.create_bind_group_layout.entries[1].binding_type ==
               DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER;
}



/**
 * Find the scene common bind-group layout id in an emitted stream.
 *
 * @param stream the emitted DRP2 stream
 * @return the common bind-group layout id, or zero when absent
 */
uint64_t _stream_scene_common_layout_id(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && _is_scene_common_bind_group_layout(cmd))
            return cmd->u.create_bind_group_layout.id;
    }
    return 0;
}



/**
 * Find the layout id used by a bind group in an emitted stream.
 *
 * @param stream the emitted DRP2 stream
 * @param bind_group_id the bind group id
 * @return the bind group's layout id, or zero when absent
 */
uint64_t
_stream_bind_group_layout_id(const DvzDrp2CommandStream* stream, uint64_t bind_group_id)
{
    ANN(stream);
    if (bind_group_id == 0)
        return 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP &&
            cmd->u.create_bind_group.id == bind_group_id)
        {
            return cmd->u.create_bind_group.bind_group_layout_id;
        }
    }
    return 0;
}



/**
 * Return whether an emitted stream contains one render pipeline debug label.
 *
 * @param stream the emitted DRP2 stream
 * @param label expected pipeline label
 * @return whether the pipeline label was found
 */
bool _stream_has_render_pipeline_label(const DvzDrp2CommandStream* stream, const char* label)
{
    ANN(stream);
    ANN(label);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* pipeline_label =
            dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
        if (pipeline_label != NULL && strcmp(pipeline_label, label) == 0)
            return true;
    }
    return false;
}



/**
 * Return whether an emitted stream contains a render pipeline label substring.
 *
 * @param stream the emitted DRP2 stream
 * @param part expected pipeline label substring
 * @return whether a matching pipeline label was found
 */
bool _stream_has_render_pipeline_label_part(const DvzDrp2CommandStream* stream, const char* part)
{
    ANN(stream);
    ANN(part);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* pipeline_label =
            dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
        if (pipeline_label != NULL && strstr(pipeline_label, part) != NULL)
            return true;
    }
    return false;
}



/**
 * Register the shared scene-graph GPU fixture.
 *
 * @param suite the active test suite
 */
void _scene_graph_register_gpu_fixture(TstSuite* suite)
{
    ANN(suite);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    tst_suite_register_fixture(
        suite, TST_SCENE_GRAPH_GPU_FIXTURE, TST_FIXTURE_SCOPE_PROCESS,
        _scene_graph_gpu_fixture_create, _scene_graph_gpu_fixture_destroy);
#else
    (void)suite;
#endif
}
