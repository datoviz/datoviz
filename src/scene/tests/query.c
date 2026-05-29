/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query tests                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_assertions.h"
#include "../query/internal.h"
#include "datoviz/drp2.h"
#include "datoviz/scene/panzoom.h"
#include "datoviz/vk/gpu_ctx.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TST_SCENE_QUERY_GPU_CASE(test)                                                            \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_SCENE_QUERY_REQUIRE_VKLITE(ctx)                                                       \
    do                                                                                            \
    {                                                                                             \
        if (!_scene_vklite_runtime_available())                                                   \
        {                                                                                         \
            tst_skip((ctx), "Vulkan instance creation failed");                                   \
            return 0;                                                                             \
        }                                                                                         \
    } while (0)



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_query_registry_covers_active_visual_families(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const DvzSceneVisualFamily families[] = {
        DVZ_SCENE_VISUAL_FAMILY_POINT,     DVZ_SCENE_VISUAL_FAMILY_PIXEL,
        DVZ_SCENE_VISUAL_FAMILY_MARKER,    DVZ_SCENE_VISUAL_FAMILY_SPHERE,
        DVZ_SCENE_VISUAL_FAMILY_VECTOR,    DVZ_SCENE_VISUAL_FAMILY_SEGMENT,
        DVZ_SCENE_VISUAL_FAMILY_PATH,
        DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE, DVZ_SCENE_VISUAL_FAMILY_MESH,
        DVZ_SCENE_VISUAL_FAMILY_IMAGE,     DVZ_SCENE_VISUAL_FAMILY_LABELS,
        DVZ_SCENE_VISUAL_FAMILY_VOLUME,    DVZ_SCENE_VISUAL_FAMILY_TEXT,
        DVZ_SCENE_VISUAL_FAMILY_GLYPH,
    };
    const uint32_t family_count = sizeof(families) / sizeof(families[0]);
    AT(_dvz_scene_query_registry_count() == family_count);

    for (uint32_t i = 0; i < family_count; i++)
    {
        const DvzSceneQueryFamilyOps* ops = _dvz_scene_query_registry_find(families[i]);
        ANN(ops);
        AT(ops->family == families[i]);
        AT(ops->name != NULL);
    }
    AT(_dvz_scene_query_registry_find(DVZ_SCENE_VISUAL_FAMILY_NONE) == NULL);
    return 0;
}



int test_scene_query_rejects_untyped_render_plan(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzFramePlan* plan = dvz_frame_plan("figure.query.untyped_metadata", 1);
    ANN(plan);
    AT(dvz_frame_plan_render_metadata_complete(plan));

    AT(dvz_frame_plan_render(plan, "panel.query", "target.query", true));
    AT(dvz_frame_plan_render_visual(plan, "query0"));
    AT(!dvz_frame_plan_render_metadata_complete(plan));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    AT(dvz_frame_plan_render_metadata_complete(plan));

    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_scene_query_queue_processes_native_results(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.25f, .y = 0.5f, .width = 0.5f, .height = 0.25f});
    ANN(panel);

    AT(dvz_panel_query(
           panel, 10.0, 20.0,
           &(DvzQueryRequest){.request_id = 11, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(scene->pending_query_count == 1);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);
    AT(scene->pending_query_count == 0);
    AT(scene->query_result_count == 1);

    DvzQueryResult result = {0};
    AT(dvz_scene_poll_query(scene, &result));
    AT(result.request_id == 11);
    AT(result.status == DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL);
    AT(result.panel_id == 1);
    AC(result.panel_position[0], 10.0, 1e-12);
    AC(result.panel_position[1], 20.0, 1e-12);
    AT(result.framebuffer_position[0] == 60);
    AT(result.framebuffer_position[1] == 70);
    AT(!dvz_scene_poll_query(scene, &result));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_query_queue_coalesces_pending_requests(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_query(
           panel, 8.0, 8.0,
           &(DvzQueryRequest){.request_id = 12, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_panel_query(
           panel, 16.0, 16.0,
           &(DvzQueryRequest){.request_id = 12, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(scene->pending_query_count == 1);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);

    DvzQueryResult result = {0};
    AT(dvz_scene_poll_query(scene, &result));
    AT(result.request_id == 12);
    AC(result.panel_position[0], 16.0, 1e-12);
    AC(result.panel_position[1], 16.0, 1e-12);
    AT(!dvz_scene_poll_query(scene, &result));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_query_volume_sample_is_explicitly_unsupported(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_SAMPLE);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 33, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, NULL, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 33);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY);
    AT(query.visual_id == _scene_visual_public_id(scene, volume));
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_query_skips_fixed_visuals(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    dvz_visual_set_query_capabilities(point, DVZ_QUERY_CAPABILITY_ITEM);
    AT(dvz_panel_add_visual(
           panel, point, &(DvzVisualAttachDesc){.controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 37, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 37);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL);
    AT(query.visual_id == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure query processing fails explicitly when no supported profile remains.
 *
 * @param suite test context
 * @param item test case
 * @return 0 on success
 */
int test_scene_query_rejects_missing_query_profile(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    dvz_visual_set_query_capabilities(point, DVZ_QUERY_CAPABILITY_ITEM);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 38, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.query_profile_u32_r32 = false;
    caps.query_profile_u64_rg32 = false;
    caps.query_profile_u64_2xr32 = false;
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 38);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE);
    AT(query.profile == DVZ_QUERY_PROFILE_UNSUPPORTED);
    AT(query.visual_id == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure automatic profile selection does not choose the deferred two-attachment query profile.
 *
 * @param suite test context
 * @param item test case
 * @return 0 on success
 */
int test_scene_query_does_not_auto_select_2xr32_profile(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    dvz_visual_set_query_capabilities(point, DVZ_QUERY_CAPABILITY_ITEM);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 40, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.query_profile_u32_r32 = false;
    caps.query_profile_u64_rg32 = false;
    caps.query_profile_u64_2xr32 = true;
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 40);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE);
    AT(query.profile == DVZ_QUERY_PROFILE_UNSUPPORTED);
    AT(query.visual_id == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure family execution rejects a requested profile it cannot decode.
 *
 * @param suite test context
 * @param item test case
 * @return 0 on success
 */
int test_scene_query_rejects_family_unsupported_profile(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    dvz_visual_set_query_capabilities(point, DVZ_QUERY_CAPABILITY_ITEM);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){
               .request_id = 39,
               .target = DVZ_SCENE_TARGET_ITEM,
               .profile = DVZ_QUERY_PROFILE_U64_RG32,
           }) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, NULL, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 39);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE);
    AT(query.profile == DVZ_QUERY_PROFILE_U64_RG32);
    AT(query.visual_id == _scene_visual_public_id(scene, point));
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure image sample queries resolve through the native GPU value path.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_query_resolves_sample(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_SAMPLE);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 64;
        pixels[4 * i + 1] = 128;
        pixels[4 * i + 2] = 255;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 35, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 35);
    AT(query.hit);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_id == _scene_visual_public_id(scene, image));
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(query.raw_target == DVZ_SCENE_TARGET_SAMPLE);
    AT(query.resolved_target == DVZ_SCENE_TARGET_SAMPLE);
    AT(query.value_kind == DVZ_QUERY_VALUE_VEC4);
    AC(query.vector[0], 64.0 / 255.0, 1e-3);
    AC(query.vector[1], 128.0 / 255.0, 1e-3);
    AT(query.vector[2] > 0.9);
    AT(query.vector[3] > 0.9);
    AT(query.has_display_rgba);
    AC(query.display_rgba[0], query.vector[0], 1e-12);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure generated image-rectangle sample queries use image geometry in the DRP2 stream.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_query_generated_rect_samples_position(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 extent[1] = {{1.0f, 1.0f}};
    vec2 anchor[1] = {{0.0f, 0.0f}};
    uint8_t pixels[8 * 8 * 4] = {0};
    for (uint32_t y = 0; y < 8; y++)
    {
        for (uint32_t x = 0; x < 8; x++)
        {
            uint32_t i = 4u * (y * 8u + x);
            if (x < 4)
                pixels[i + 0] = 255;
            else
                pixels[i + 2] = 255;
            pixels[i + 3] = 255;
        }
    }
    AT(dvz_visual_set_data(image, "position", position, 1) == 0);
    AT(dvz_visual_set_data(image, "extent", extent, 1) == 0);
    AT(dvz_visual_set_data(image, "anchor", anchor, 1) == 0);
    AT(dvz_visual_set_texture(image, pixels, 8, 8) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 44.0, 32.0,
           &(DvzQueryRequest){.request_id = 135, .target = DVZ_SCENE_TARGET_PIXEL}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 135);
    AT(query.hit);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(query.resolved_target == DVZ_SCENE_TARGET_PIXEL);
    AT(query.value_kind == DVZ_QUERY_VALUE_VEC4);
    AT(query.vector[0] < 0.2);
    AT(query.vector[2] > 0.8);
    AT(query.vector[3] > 0.9);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure image pixel queries apply the same panzoom transform as rendered images.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_query_panzoom_samples_transformed_position(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) == 0);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    ANN(panzoom);
    dvz_panzoom_zoom(panzoom, (vec2){2.0f, 2.0f});
    dvz_panzoom_pan(panzoom, (vec2){0.0f, 0.2f});

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[8 * 8 * 4] = {0};
    for (uint32_t y = 0; y < 8; y++)
    {
        for (uint32_t x = 0; x < 8; x++)
        {
            uint32_t i = 4u * (y * 8u + x);
            if (y < 4)
                pixels[i + 0] = 255;
            else
                pixels[i + 2] = 255;
            pixels[i + 3] = 255;
        }
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 8, 8) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    for (uint64_t request_id = 140; request_id <= 141; request_id++)
    {
        AT(dvz_panel_query(
               panel, 32.0, 16.0,
               &(DvzQueryRequest){.request_id = request_id, .target = DVZ_SCENE_TARGET_PIXEL}) ==
           0);
        AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

        DvzQueryResult query = {0};
        AT(dvz_scene_poll_query(scene, &query));
        AT(query.request_id == request_id);
        AT(query.hit);
        AT(query.status == DVZ_QUERY_STATUS_HIT);
        AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
        AT(query.resolved_target == DVZ_SCENE_TARGET_PIXEL);
        AT(query.value_kind == DVZ_QUERY_VALUE_VEC4);
        AT(query.vector[0] < 0.2);
        AT(query.vector[2] > 0.8);
        AT(query.vector[3] > 0.9);
        AT(!dvz_scene_poll_query(scene, &query));
    }

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure repeated image queries reuse the retained scene request executor.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_query_reuses_retained_request_executor(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_SAMPLE);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 64;
        pixels[4 * i + 1] = 128;
        pixels[4 * i + 2] = 255;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 601, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);
    AT(scene->query_executor.runtime_create_count == 1);
    AT(scene->query_executor.emitter_create_count == 1);
    AT(scene->query_executor.query_static_cache_upload_count == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 601);
    AT(query.hit);
    AT(!dvz_scene_poll_query(scene, &query));

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 602, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);
    AT(scene->query_executor.runtime_create_count == 1);
    AT(scene->query_executor.emitter_create_count == 1);
    AT(scene->query_executor.query_static_cache_upload_count == 1);

    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 602);
    AT(query.hit);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure image sample queries fail explicitly when GPU readback fails.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_sample_query_readback_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    scene->test.force_readback_download_failure = true;
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_SAMPLE);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 1] = 255;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 36, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 36);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_READBACK_FAILED);
    AT(query.visual_id == _scene_visual_public_id(scene, image));
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native item queries miss without falling back to a legacy identity adapter.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_query_misses_empty_pixel(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* points = dvz_point(scene, 0);
    ANN(points);
    dvz_visual_set_query_capabilities(points, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 255, 255, 255}};
    float size[1] = {8.0f};
    AT(dvz_visual_set_data(points, "position", position, 1) == 0);
    AT(dvz_visual_set_data(points, "color", color, 1) == 0);
    AT(dvz_visual_set_data(points, "size", size, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 2.0, 2.0,
           &(DvzQueryRequest){.request_id = 34, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 34);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_MISS);
    AT(query.visual_id == _scene_visual_public_id(scene, points));
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_scene_pixel_query_accepts_square_corner(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* pixel = dvz_pixel(scene, 0);
    ANN(pixel);
    dvz_visual_set_query_capabilities(pixel, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 pixel_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor pixel_color[1] = {{255, 255, 255, 255}};
    float pixel_size[1] = {24.0f};
    AT(dvz_visual_set_data(pixel, "position", pixel_pos, 1) == 0);
    AT(dvz_visual_set_data(pixel, "color", pixel_color, 1) == 0);
    AT(dvz_visual_set_data(pixel, "size", pixel_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 42.0, 42.0,
           &(DvzQueryRequest){.request_id = 52, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 52);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_PIXEL);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 0);
    AT(query.item_id == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_scene_marker_query_accepts_bbox_corner(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* marker = dvz_marker(scene, 0);
    ANN(marker);
    dvz_visual_set_query_capabilities(marker, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 marker_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor marker_color[1] = {{255, 255, 255, 255}};
    float marker_size[1] = {24.0f};
    float marker_angle[1] = {0.0f};
    uint32_t marker_shape[1] = {DVZ_MARKER_SHAPE_DISC};
    AT(dvz_visual_set_data(marker, "position", marker_pos, 1) == 0);
    AT(dvz_visual_set_data(marker, "color", marker_color, 1) == 0);
    AT(dvz_visual_set_data(marker, "size", marker_size, 1) == 0);
    AT(dvz_visual_set_data(marker, "angle", marker_angle, 1) == 0);
    AT(dvz_visual_set_data(marker, "shape", marker_shape, 1) == 0);
    AT(dvz_panel_add_visual(panel, marker, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 42.0, 42.0,
           &(DvzQueryRequest){.request_id = 54, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 54);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_MARKER);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 0);
    AT(query.item_id == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_scene_sphere_query_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(sphere);
    dvz_visual_set_query_capabilities(sphere, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 positions[2] = {
        {-0.5f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
    };
    DvzColor colors[2] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
    };
    float radii[2] = {0.2f, 0.25f};
    DvzVisualDataUpdate sphere_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 2},
        {.attr_name = "color", .data = colors, .item_count = 2},
        {.attr_name = "radius", .data = radii, .item_count = 2},
    };
    AT(dvz_visual_set_data_many(sphere, sphere_updates, 3) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 61, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 61);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_id == _scene_visual_public_id(scene, sphere));
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_SPHERE);
    AT(query.raw_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.raw_id == 1);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 1);
    AT(query.item_id == 1);
    AT(!query.has_data_position);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_scene_segment_query_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* segment = dvz_segment(scene, 0);
    ANN(segment);
    dvz_visual_set_query_capabilities(segment, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 segment_start[2] = {
        {-0.75f, -0.5f, 0.0f},
        {-0.75f, 0.0f, 0.0f},
    };
    vec3 segment_end[2] = {
        {0.75f, -0.5f, 0.0f},
        {0.75f, 0.0f, 0.0f},
    };
    DvzColor segment_color[2] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float segment_width[2] = {12.0f, 12.0f};
    AT(dvz_visual_set_data(segment, "position_start", segment_start, 2) == 0);
    AT(dvz_visual_set_data(segment, "position_end", segment_end, 2) == 0);
    AT(dvz_visual_set_data(segment, "color", segment_color, 2) == 0);
    AT(dvz_visual_set_data(segment, "line_width", segment_width, 2) == 0);
    AT(dvz_segment_set_caps(segment, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) == 0);
    AT(dvz_panel_add_visual(panel, segment, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 71, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 71);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_SEGMENT);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 1);
    AT(query.item_id == 1);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native path queries resolve stroked segment identity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_path_query_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* path = dvz_path(scene, 0);
    ANN(path);
    dvz_visual_set_query_capabilities(path, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 path_position[3] = {
        {-0.75f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.75f, 0.0f, 0.0f},
    };
    DvzColor path_color[3] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float path_width[3] = {12.0f, 12.0f, 12.0f};
    AT(dvz_visual_set_data(path, "position", path_position, 3) == 0);
    AT(dvz_visual_set_data(path, "color", path_color, 3) == 0);
    AT(dvz_visual_set_data(path, "line_width", path_width, 3) == 0);
    AT(dvz_path_set_caps(path, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) == 0);
    AT(dvz_path_set_join(path, DVZ_PATH_JOIN_MITER, 4.0f) == 0);
    AT(dvz_panel_add_visual(panel, path, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 48.0, 32.0,
           &(DvzQueryRequest){.request_id = 73, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 73);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_PATH);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 1);
    AT(query.item_id == 1);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native straight-vector queries resolve vector identity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_vector_query_resolves_straight_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* vector = dvz_vector(scene, 0);
    ANN(vector);
    dvz_visual_set_query_capabilities(vector, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 position[2] = {
        {-0.75f, -0.5f, 0.0f},
        {-0.75f, 0.0f, 0.0f},
    };
    vec3 displacement[2] = {
        {1.5f, 0.0f, 0.0f},
        {1.5f, 0.0f, 0.0f},
    };
    DvzColor color[2] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float width[2] = {12.0f, 12.0f};
    AT(dvz_visual_set_data(vector, "position", position, 2) == 0);
    AT(dvz_visual_set_data(vector, "vector", displacement, 2) == 0);
    AT(dvz_visual_set_data(vector, "color", color, 2) == 0);
    AT(dvz_visual_set_data(vector, "stroke_width", width, 2) == 0);
    DvzVectorStyle style = dvz_vector_style();
    style.start_cap = DVZ_SEGMENT_CAP_BUTT;
    style.end_cap = DVZ_SEGMENT_CAP_BUTT;
    AT(dvz_vector_set_style(vector, &style) == 0);
    AT(dvz_panel_add_visual(panel, vector, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 75, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 75);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_VECTOR);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 1);
    AT(query.item_id == 1);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native curved-vector queries resolve vector identity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_vector_query_resolves_curved_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* vector = dvz_vector(scene, 0);
    ANN(vector);
    dvz_visual_set_query_capabilities(vector, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 position[3] = {
        {-0.75f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.75f, 0.0f, 0.0f},
    };
    DvzColor color[3] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float width[3] = {12.0f, 12.0f, 12.0f};
    uint32_t subpaths[1] = {3};
    AT(dvz_visual_set_data(vector, "position", position, 3) == 0);
    AT(dvz_visual_set_data(vector, "color", color, 3) == 0);
    AT(dvz_visual_set_data(vector, "stroke_width", width, 3) == 0);
    AT(dvz_vector_set_subpaths(vector, 1, subpaths) == 0);
    DvzVectorStyle style = dvz_vector_style();
    style.start_cap = DVZ_SEGMENT_CAP_BUTT;
    style.end_cap = DVZ_SEGMENT_CAP_BUTT;
    AT(dvz_vector_set_style(vector, &style) == 0);
    AT(dvz_panel_add_visual(panel, vector, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 48.0, 32.0,
           &(DvzQueryRequest){.request_id = 77, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 77);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_VECTOR);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 1);
    AT(query.item_id == 1);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native primitive queries resolve primitive identity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_primitive_query_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(primitive);
    dvz_visual_set_query_capabilities(primitive, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 primitive_pos[6] = {
        {-0.9f, -0.7f, 0.0f},
        {-0.9f, 0.7f, 0.0f},
        {-0.1f, 0.0f, 0.0f},
        {0.1f, -0.7f, 0.0f},
        {0.1f, 0.7f, 0.0f},
        {0.9f, 0.0f, 0.0f},
    };
    DvzColor primitive_color[6] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    DvzVisualDataUpdate primitive_updates[] = {
        {.attr_name = "position", .data = primitive_pos, .item_count = 6},
        {.attr_name = "color", .data = primitive_color, .item_count = 6},
    };
    AT(dvz_visual_set_data_many(primitive, primitive_updates, 2) == 0);
    AT(dvz_panel_add_visual(panel, primitive, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 48.0, 32.0,
           &(DvzQueryRequest){.request_id = 81, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 81);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 1);
    AT(query.item_id == 1);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native mesh queries resolve indexed triangle identity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_mesh_query_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);
    dvz_visual_set_query_capabilities(mesh, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 mesh_pos[4] = {
        {-0.8f, -0.8f, 0.0f},
        {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},
        {0.8f, 0.8f, 0.0f},
    };
    vec3 mesh_normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_indices, sizeof(mesh_indices)));
    DvzVisualDataUpdate mesh_updates[] = {
        {.attr_name = "position", .data = mesh_pos, .item_count = 4},
        {.attr_name = "normal", .data = mesh_normals, .item_count = 4},
    };
    AT(dvz_visual_set_data_many(mesh, mesh_updates, 2) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 48.0, 32.0,
           &(DvzQueryRequest){.request_id = 83, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 83);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_MESH);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 1);
    AT(query.item_id == 1);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native image item queries resolve generated image-quad identity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_query_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 positions[2] = {
        {-0.5f, 0.0f, 0.0f},
        {0.5f, 0.0f, 0.0f},
    };
    vec2 extents[2] = {
        {0.4f, 0.4f},
        {0.4f, 0.4f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(image, "extent", extents, 2) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 48.0, 32.0,
           &(DvzQueryRequest){.request_id = 91, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 91);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 1);
    AT(query.item_id == 1);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native volume item queries resolve visual identity through proxy geometry.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_query_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_ITEM);
    double bounds_min[3] = {-0.4, -0.4, -0.4};
    double bounds_max[3] = {+0.4, +0.4, +0.4};
    AT(dvz_volume_set_bounds(volume, bounds_min, bounds_max) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 111, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 111);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_VOLUME);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 0);
    AT(query.item_id == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native volume slice sample queries return scalar values from the GPU.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_query_resolves_sample(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_SAMPLE);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_NEAREST) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {128, 128, 128, 128, 128, 128, 128, 128};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 112, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 112);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_VOLUME);
    AT(query.resolved_target == DVZ_SCENE_TARGET_SAMPLE);
    AT(query.value_kind == DVZ_QUERY_VALUE_SCALAR);
    AC(query.scalar, 128.0 / 255.0, 1e-3);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native label-volume sample queries return categorical IDs and scale labels.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_query_resolves_label_sample(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_SAMPLE);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_NEAREST) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint16_t voxels[8] = {23, 23, 23, 23, 23, 23, 23, 23};
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = voxels,
            .bytes_per_row = 2 * sizeof(uint16_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL, .label = "regions"});
    ANN(scale);
    DvzScaleCategory category = {
        .category_id = 23,
        .order = 0,
        .label = "region 23",
        .color = {20, 120, 220, 255},
    };
    AT(dvz_scale_set_categories(scale, &category, 1));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 117, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 117);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_VOLUME);
    AT(query.resolved_target == DVZ_SCENE_TARGET_SAMPLE);
    AT(query.value_kind == DVZ_QUERY_VALUE_CATEGORY);
    AT(query.category_id == 23);
    AT(query.scale == scale);
    AT(strcmp(query.label, "region 23") == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


/**
 * Exercise one native label-volume sample query fixture.
 *
 * @param suite the active test suite
 * @param format retained field format
 * @param data label voxel data
 * @param bytes_per_row row stride in bytes
 * @param category_id expected category id
 * @param label expected category label
 * @param request_id query request id
 * @return 0 on success
 */
static int _test_scene_volume_query_label_sample_value(
    TstContext* suite, DvzFieldFormat format, const void* data, uint32_t bytes_per_row,
    DvzCategoryId category_id, const char* label, uint64_t request_id)
{
    ANN(suite);
    ANN(data);
    ANN(label);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_SAMPLE);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_NEAREST) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = format,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = data,
            .bytes_per_row = bytes_per_row,
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL, .label = "regions"});
    ANN(scale);
    DvzScaleCategory category = {
        .category_id = category_id,
        .order = 0,
        .label = label,
        .color = {20, 120, 220, 255},
    };
    AT(dvz_scale_set_categories(scale, &category, 1));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = request_id, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == request_id);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_VOLUME);
    AT(query.resolved_target == DVZ_SCENE_TARGET_SAMPLE);
    AT(query.value_kind == DVZ_QUERY_VALUE_CATEGORY);
    AT(query.category_id == category_id);
    AT(query.scale == scale);
    AT(strcmp(query.label, label) == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


/**
 * Ensure native label-volume sample queries preserve high unsigned R32 labels.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_query_resolves_high_uint_label_sample(
    TstContext* suite, const TstCase* item)
{
    (void)item;
    const uint32_t voxels[8] = {
        UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX,
        UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
    return _test_scene_volume_query_label_sample_value(
        suite, DVZ_FIELD_FORMAT_R32_UINT, voxels, 2 * sizeof(uint32_t),
        (DvzCategoryId)UINT32_MAX, "region uint32 max", 118);
}


/**
 * Ensure native label-volume sample queries preserve signed negative R32 labels.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_query_resolves_signed_label_sample(TstContext* suite, const TstCase* item)
{
    (void)item;
    const int32_t voxels[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    return _test_scene_volume_query_label_sample_value(
        suite, DVZ_FIELD_FORMAT_R32_SINT, voxels, 2 * sizeof(int32_t), -1,
        "region negative one", 119);
}



/**
 * Ensure requested rg32 volume sample queries return GPU-computed UVW.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_query_resolves_sample_uvw_profile(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_SAMPLE);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_NEAREST) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {128, 128, 128, 128, 128, 128, 128, 128};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){
               .request_id = 116,
               .target = DVZ_SCENE_TARGET_SAMPLE,
               .profile = DVZ_QUERY_PROFILE_U64_RG32,
           }) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 116);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.profile == DVZ_QUERY_PROFILE_U64_RG32);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_VOLUME);
    AT(query.resolved_target == DVZ_SCENE_TARGET_SAMPLE);
    AT(query.value_kind == DVZ_QUERY_VALUE_SCALAR);
    AC(query.scalar, 128.0 / 255.0, 1e-3);
    AT(query.has_uvw);
    AC(query.uvw[0], 0.5, 0.01);
    AC(query.uvw[1], 0.5, 0.01);
    AC(query.uvw[2], 0.5, 0.01);
    AT(query.resolved_id == 7);
    AT(query.voxel_id == 7);
    AT(query.texel_id == 7);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure deferred volume sample policies stay explicitly unsupported.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_sample_query_rejects_deferred_policies(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_SAMPLE);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {128, 128, 128, 128, 128, 128, 128, 128};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 113, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 113);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY);
    AT(query.visual_id == _scene_visual_public_id(scene, volume));
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure unsupported volume sample formats stay explicitly unsupported.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_sample_query_rejects_unsupported_format(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_SAMPLE);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 114, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 114);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY);
    AT(query.visual_id == _scene_visual_public_id(scene, volume));
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure volume sample queries fail explicitly when GPU readback fails.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_sample_query_readback_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_query_capabilities(volume, DVZ_QUERY_CAPABILITY_SAMPLE);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {128, 128, 128, 128, 128, 128, 128, 128};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    scene->test.force_readback_download_failure = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 115, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 115);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_READBACK_FAILED);
    AT(query.visual_id == _scene_visual_public_id(scene, volume));
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_VOLUME);
    AT(query.resolved_target == DVZ_SCENE_TARGET_SAMPLE);
    AT(query.value_kind == DVZ_QUERY_VALUE_NONE);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native labels queries return signed integer IDs from the rendered labels pass.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_labels_query_resolves_category(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    dvz_visual_set_query_capabilities(labels, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 positions[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(labels, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(labels, "texcoords", texcoords, 4) == 0);

    int32_t label_data[4 * 4] = {0};
    label_data[3 * 4 + 1] = -7;
    label_data[3 * 4 + 3] = 17;
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = label_data,
                   .bytes_per_row = 4 * sizeof(int32_t),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(labels, "field", field));
    AT(dvz_labels_set_background(labels, 0) == 0);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL, .label = "segments"});
    ANN(scale);
    DvzScaleCategory categories[2] = {
        {.category_id = -7, .order = 0, .label = "negative seven", .color = {255, 0, 0, 255}},
        {.category_id = 17, .order = 1, .label = "seventeen", .color = {0, 255, 0, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));
    AT(dvz_visual_set_scale(labels, "labels", scale) == 0);
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 16.0, 16.0,
           &(DvzQueryRequest){.request_id = 141, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_panel_query(
           panel, 48.0, 16.0,
           &(DvzQueryRequest){.request_id = 142, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_panel_query(
           panel, 16.0, 48.0,
           &(DvzQueryRequest){.request_id = 143, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 3);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 141);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_LABELS);
    AT(query.resolved_target == DVZ_SCENE_TARGET_SEGMENT);
    AT(query.category_id == -7);
    AT(query.value_kind == DVZ_QUERY_VALUE_CATEGORY);
    AT(query.scale == scale);
    AT(strcmp(query.label, "negative seven") == 0);
    AT(query.has_uvw);
    AT(query.uvw[0] > 0.2 && query.uvw[0] < 0.3);
    AT(query.uvw[1] > 0.7 && query.uvw[1] < 0.8);

    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 142);
    AT(query.category_id == 17);
    AT(strcmp(query.label, "seventeen") == 0);

    AT(dvz_scene_poll_query(scene, &query));
    AT(!query.hit);
    AT(query.request_id == 143);
    AT(query.status == DVZ_QUERY_STATUS_MISS);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Ensure native labels queries preserve high unsigned 32-bit category IDs.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_labels_query_high_unsigned_id(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    dvz_visual_set_query_capabilities(labels, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 positions[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(labels, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(labels, "texcoords", texcoords, 4) == 0);

    uint32_t label_data[4 * 4] = {0};
    label_data[3 * 4 + 3] = 4000000000u;
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = label_data,
                   .bytes_per_row = 4 * sizeof(uint32_t),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(labels, "field", field));
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 48.0, 16.0,
           &(DvzQueryRequest){.request_id = 145, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.hit);
    AT(query.request_id == 145);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_LABELS);
    AT(query.category_id == 4000000000LL);
    AT(query.value_kind == DVZ_QUERY_VALUE_CATEGORY);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


/**
 * Ensure labels queries reject visuals without a queryable label field explicitly.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_labels_query_rejects_missing_field(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    dvz_visual_set_query_capabilities(labels, DVZ_QUERY_CAPABILITY_ITEM);
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 146, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 146);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY);
    AT(query.visual_id == _scene_visual_public_id(scene, labels));
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure labels query rejects non-integer label field formats explicitly.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_labels_query_rejects_unsupported_format(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    dvz_visual_set_query_capabilities(labels, DVZ_QUERY_CAPABILITY_ITEM);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    _visual_family_state(labels)->field = field;
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 153, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, NULL, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 153);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY);
    AT(query.visual_id == _scene_visual_public_id(scene, labels));
    AT(query.value_kind == DVZ_QUERY_VALUE_NONE);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure labels queries fail explicitly when GPU readback fails.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_labels_query_readback_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    scene->test.force_readback_download_failure = true;
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    dvz_visual_set_query_capabilities(labels, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 positions[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(labels, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(labels, "texcoords", texcoords, 4) == 0);

    int32_t label_data[4 * 4] = {0};
    label_data[3 * 4 + 3] = 17;
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = label_data,
                   .bytes_per_row = 4 * sizeof(int32_t),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(labels, "field", field));
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 48.0, 16.0,
           &(DvzQueryRequest){.request_id = 147, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 1);

    DvzQueryResult query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    AT(query.request_id == 147);
    AT(!query.hit);
    AT(query.status == DVZ_QUERY_STATUS_READBACK_FAILED);
    AT(query.visual_id == _scene_visual_public_id(scene, labels));
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_LABELS);
    AT(!dvz_scene_poll_query(scene, &query));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_scene_query_processes_item_and_pixel_results(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_QUERY_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* points = dvz_point(scene, 0);
    ANN(points);
    dvz_visual_set_query_capabilities(points, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 point_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 0, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(points, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, &(DvzVisualAttachDesc){.z_layer = -1}) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 101, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_panel_query(
           panel, 32.0, 32.0,
           &(DvzQueryRequest){.request_id = 102, .target = DVZ_SCENE_TARGET_PIXEL}) == 0);
    AT(dvz_figure_process_queries(figure, runtime, &caps) == 2);

    DvzQueryResult item_result = {0};
    DvzQueryResult pixel = {0};
    AT(dvz_scene_poll_query(scene, &item_result));
    AT(dvz_scene_poll_query(scene, &pixel));
    AT(item_result.hit);
    AT(item_result.request_id == 101);
    AT(item_result.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT);
    AT(item_result.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(item_result.item_id == 0);
    AT(pixel.hit);
    AT(pixel.request_id == 102);
    AT(pixel.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(pixel.resolved_target == DVZ_SCENE_TARGET_PIXEL);
    AT(pixel.value_kind == DVZ_QUERY_VALUE_VEC4);
    AT(pixel.vector[0] > 0.9);
    AT(pixel.vector[1] < 0.1);
    AT(pixel.vector[2] < 0.1);
    AT(pixel.vector[3] > 0.9);
    AT(!dvz_scene_poll_query(scene, &pixel));

    dvz_scene_destroy(scene);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Register scene query tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_query(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("query");

    TST_CASE(test_scene_query_registry_covers_active_visual_families);
    TST_CASE(test_scene_query_rejects_untyped_render_plan);
    TST_CASE(test_scene_query_queue_processes_native_results);
    TST_CASE(test_scene_query_queue_coalesces_pending_requests);
    TST_CASE(test_scene_query_volume_sample_is_explicitly_unsupported);
    TST_CASE(test_scene_query_skips_fixed_visuals);
    TST_CASE(test_scene_query_rejects_missing_query_profile);
    TST_CASE(test_scene_query_does_not_auto_select_2xr32_profile);
    TST_CASE(test_scene_query_rejects_family_unsupported_profile);
    TST_SCENE_QUERY_GPU_CASE(test_scene_image_query_resolves_sample);
    TST_SCENE_QUERY_GPU_CASE(test_scene_image_query_generated_rect_samples_position);
    TST_SCENE_QUERY_GPU_CASE(test_scene_image_query_panzoom_samples_transformed_position);
    TST_SCENE_QUERY_GPU_CASE(test_scene_image_query_reuses_retained_request_executor);
    TST_SCENE_QUERY_GPU_CASE(test_scene_image_sample_query_readback_failure);
    TST_SCENE_QUERY_GPU_CASE(test_scene_point_query_misses_empty_pixel);
    TST_SCENE_QUERY_GPU_CASE(test_scene_pixel_query_accepts_square_corner);
    TST_SCENE_QUERY_GPU_CASE(test_scene_marker_query_accepts_bbox_corner);
    TST_SCENE_QUERY_GPU_CASE(test_scene_sphere_query_resolves_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_segment_query_resolves_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_path_query_resolves_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_vector_query_resolves_straight_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_vector_query_resolves_curved_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_primitive_query_resolves_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_mesh_query_resolves_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_image_query_resolves_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_volume_query_resolves_item);
    TST_SCENE_QUERY_GPU_CASE(test_scene_volume_query_resolves_sample);
    TST_SCENE_QUERY_GPU_CASE(test_scene_volume_query_resolves_label_sample);
    TST_SCENE_QUERY_GPU_CASE(test_scene_volume_query_resolves_high_uint_label_sample);
    TST_SCENE_QUERY_GPU_CASE(test_scene_volume_query_resolves_signed_label_sample);
    TST_SCENE_QUERY_GPU_CASE(test_scene_volume_query_resolves_sample_uvw_profile);
    TST_CASE(test_scene_volume_sample_query_rejects_deferred_policies);
    TST_CASE(test_scene_volume_sample_query_rejects_unsupported_format);
    TST_SCENE_QUERY_GPU_CASE(test_scene_volume_sample_query_readback_failure);
    TST_SCENE_QUERY_GPU_CASE(test_scene_labels_query_resolves_category);
    TST_SCENE_QUERY_GPU_CASE(test_scene_labels_query_high_unsigned_id);
    TST_CASE(test_scene_labels_query_rejects_missing_field);
    TST_CASE(test_scene_labels_query_rejects_unsupported_format);
    TST_SCENE_QUERY_GPU_CASE(test_scene_labels_query_readback_failure);
    TST_SCENE_QUERY_GPU_CASE(test_scene_query_processes_item_and_pixel_results);

    return 0;
}
