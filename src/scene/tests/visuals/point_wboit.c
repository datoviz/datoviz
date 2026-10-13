/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Point accumulation contracts                                                                 */
/*************************************************************************************************/

#include "common.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "_visual_pipeline_internal.h"



/**
 * Check every point accumulator's source, layout, and unsupported family guards.
 *
 * @param suite active test context
 * @param item active test item
 * @return 0 on success
 */
int test_scene_point_wboit_shader_contract(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    for (uint32_t variant = 0; variant < 4; variant++)
    {
        DvzSceneVisualDesc visual = {
            .kind = DVZ_SCENE_VISUAL_DESC_POINT,
            .visual_type = DVZ_VISUAL_TYPE_POINT,
            .point_like_kind = DVZ_SCENE_POINT_LIKE_POINT,
            .point_style_enabled = (variant & 1) != 0,
            .depth_cue_enabled = (variant & 2) != 0,
            .depth_test_enabled = true,
            .depth_compare_op = DVZ_COMPARE_OP_LESS_OR_EQUAL,
            .material_buffer_id = variant != 0 ? 1 : 0,
            .vertex_count = 2,
        };
        DvzSceneVisualShaderDesc shader = {0};
        AT(_scene_visual_shader_desc_resolve(&visual, false, true, "_wgsl", &shader));
        ANN(shader.vertex_glsl);
        ANN(shader.fragment_glsl);
        ANN(shader.vertex_wgsl);
        ANN(shader.fragment_wgsl);
        AT(strstr(shader.fragment_key, "_wboit") != NULL);
        AT(strstr(shader.pipeline_key, "_wboit") != NULL);
        AT(strstr(shader.fragment_glsl, "pointDiscCoverage") != NULL);
        AT(strstr(shader.fragment_wgsl, "point_disc_coverage") != NULL);
        AT(strstr(shader.fragment_glsl, "sceneWboitAccum(outColor)") != NULL);
        AT(strstr(shader.fragment_wgsl, "scene_wboit_accum(surface)") != NULL);
        AT(strstr(shader.fragment_glsl, "outColor.a <= 0.0") != NULL);
        AT(strstr(shader.fragment_wgsl, "surface.a <= 0.0") != NULL);
        if ((variant & 1) != 0)
        {
            AT(strstr(shader.fragment_glsl, "pointStyleSurface()") != NULL);
            AT(strstr(shader.fragment_wgsl, "point_style_surface(input.color, dist)") != NULL);
            AT(strstr(shader.fragment_wgsl,
                "select(edge_color, mix(linear_color, edge_color, stroke_mask), filled)") != NULL);
        }
        if ((variant & 2) != 0)
        {
            AT(strstr(shader.fragment_glsl, "applyDepthCue") != NULL);
            AT(strstr(shader.fragment_wgsl, "apply_depth_cue") != NULL);
        }
        for (uint32_t backend = 0; backend < 2; backend++)
        {
            DvzSceneVisualPipelineDesc pipeline = {0};
            AT(_scene_visual_pipeline_desc(
                &visual, false, true, true, DVZ_ALPHA_WBOIT, DVZ_CONTROLLER_APPLY,
                backend == 0 ? DVZ_SCENE_SHADER_FORMAT_GLSL : DVZ_SCENE_SHADER_FORMAT_WGSL,
                &pipeline));
            AT(pipeline.has_depth_state);
            AT(!pipeline.depth_write_enabled);
            AT(pipeline.depth_compare_op == DVZ_COMPARE_OP_LESS_OR_EQUAL);
            AT(pipeline.topology == (backend == 0 ? DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST :
                DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST));
        }
        visual.has_item_state = true;
        AT(!_scene_visual_shader_desc_resolve(&visual, false, true, "_wgsl", &shader));
    }
    AT(_scene_visual_alpha_eligibility_error(DVZ_SCENE_VISUAL_DESC_PIXEL,
        DVZ_ALPHA_WBOIT) != NULL);
    AT(_scene_visual_alpha_eligibility_error(DVZ_SCENE_VISUAL_DESC_MARKER,
        DVZ_ALPHA_WBOIT) != NULL);
    return 0;
}



/**
 * Render and capture two completed offscreen frames.
 *
 * @param view offscreen view
 * @param pixels caller-owned 64 by 64 RGBA screenshot
 * @return whether rendering and capture succeeded
 */
static bool _point_wboit_capture(DvzView* view, uint8_t* pixels)
{
    ANN(view);
    ANN(pixels);
    for (uint32_t i = 0; i < 2; i++)
        if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
            return false;
    return dvz_canvas_capture_rgba_into(
        dvz_view_canvas(view), 64, 64, pixels, 64 * 64 * 4) == DVZ_OK;
}



/**
 * Compare captures within half-float accumulation rounding tolerance.
 *
 * @param first first RGBA screenshot
 * @param second second RGBA screenshot
 * @return maximum channel delta
 */
static uint32_t _point_wboit_delta(const uint8_t* first, const uint8_t* second)
{
    uint32_t maximum = 0;
    for (uint32_t i = 0; i < 64 * 64 * 4; i++)
    {
        uint32_t delta = (uint32_t)abs((int)first[i] - (int)second[i]);
        maximum = delta > maximum ? delta : maximum;
    }
    return maximum;
}



/**
 * Prove native point coverage, accumulation, opaque depth, and multi-frame retained updates.
 *
 * @param suite active test context
 * @param item active test item
 * @return 0 on success
 */
int test_scene_point_wboit_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);
    DvzGpuCtxConfig gpu_cfg = dvz_testing_gpu_ctx_config(suite);
    VkPhysicalDeviceFeatures features10 = {.independentBlend = true};
    dvz_gpu_ctx_config_features10(&gpu_cfg, &features10);
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .timelineSemaphore = true,
    };
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = true,
        .synchronization2 = true,
    };
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* gpu_ctx = dvz_gpu_ctx(&gpu_cfg);
    if (gpu_ctx == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }
    for (uint32_t variant = 0; variant < 4; variant++)
    {
        uint8_t reference[64 * 64 * 4] = {0};
        for (uint32_t reverse = 0; reverse < 2; reverse++)
        {
            DvzScene* scene = dvz_scene();
            DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
            DvzPanel* panel = dvz_panel_full(figure);
            dvz_panel_set_background_color(panel, dvz_color_rgba(0, 0, 0, 255));
            DvzVisual* points[2] = {dvz_point(scene, 0), dvz_point(scene, 0)};
            vec3 position = {0, 0, 0.2f};
            DvzColor colors[2] = {{255, 0, 0, 128}, {0, 0, 255, 128}};
            float size = 32;
            DvzPointStyleDesc style = dvz_point_style_desc();
            style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
            style.stroke_width_px = 3;
            style.edge_color = (DvzColor){0, 255, 0, 128};
            for (uint32_t i = 0; i < 2; i++)
            {
                AT(dvz_visual_set_data(points[i], "position", &position, 1) == DVZ_OK);
                AT(dvz_visual_set_data(points[i], "color", &colors[i], 1) == DVZ_OK);
                AT(dvz_visual_set_data(points[i], "diameter_px", &size, 1) == DVZ_OK);
                AT(dvz_visual_set_alpha_mode(points[i], DVZ_ALPHA_WBOIT) == DVZ_OK);
                if ((variant & 1) != 0)
                    AT(dvz_point_set_style(points[i], &style) == DVZ_OK);
                if ((variant & 2) != 0)
                {
                    DvzDepthCueDesc cue = dvz_depth_cue_desc();
                    cue.strength = 0.5f;
                    AT(dvz_visual_set_depth_cue(points[i], &cue) == DVZ_OK);
                }
            }
            AT(dvz_panel_add_visual(panel, points[reverse], NULL) == DVZ_OK);
            AT(dvz_panel_add_visual(panel, points[1 - reverse], NULL) == DVZ_OK);
            DvzAppConfig app_cfg = dvz_app_config();
            app_cfg.enable_canvas_extensions = false;
            app_cfg.enable_glfw_extensions = false;
            DvzAppResources resources = dvz_app_resources();
            resources.gpu_ctx = gpu_ctx;
            DvzApp* app = dvz_app_with_resources(scene, &app_cfg, &resources);
            ANN(app);
            DvzView* view = dvz_view_offscreen(app, figure, 64, 64);
            ANN(view);
            uint8_t pixels[64 * 64 * 4] = {0};
            AT(_point_wboit_capture(view, pixels));
            const uint32_t center = 4 * (32 * 64 + 32);
            const uint32_t corner = 4 * (18 * 64 + 18);
            AT(pixels[center] > 60);
            AT(pixels[center + 2] > 60);
            AT(pixels[corner] == 0 && pixels[corner + 1] == 0 && pixels[corner + 2] == 0);
            if (reverse == 0)
                dvz_memcpy(reference, sizeof(reference), pixels, sizeof(pixels));
            else
                AT(_point_wboit_delta(reference, pixels) <= 2);

            // A zero-alpha point must match its hidden control, including its styled edge.
            AT(dvz_visual_set_visible(points[0], false) == DVZ_OK);
            uint8_t hidden[64 * 64 * 4] = {0};
            AT(_point_wboit_capture(view, hidden));
            colors[0].a = 0;
            AT(dvz_visual_set_data(points[0], "color", &colors[0], 1) == DVZ_OK);
            if ((variant & 1) != 0)
            {
                style.edge_color.a = 0;
                AT(dvz_point_set_style(points[0], &style) == DVZ_OK);
            }
            AT(dvz_visual_set_visible(points[0], true) == DVZ_OK);
            AT(_point_wboit_capture(view, pixels));
            AT(_point_wboit_delta(hidden, pixels) == 0);

            // Updating color, position and size removes the old footprint and draws the new one.
            colors[0] = (DvzColor){255, 0, 0, 192};
            position[0] = -0.65f;
            size = 8;
            AT(dvz_visual_set_data(points[0], "color", &colors[0], 1) == DVZ_OK);
            AT(dvz_visual_set_data(points[0], "position", &position, 1) == DVZ_OK);
            AT(dvz_visual_set_data(points[0], "diameter_px", &size, 1) == DVZ_OK);
            AT(dvz_point_set_style(points[0], NULL) == DVZ_OK);
            AT(_point_wboit_capture(view, pixels));
            AT(abs((int)pixels[center] - (int)hidden[center]) <= 1);
            const uint32_t moved = 4 * (32 * 64 + 11);
            AT(pixels[moved] > 80);

            // Grow and shrink the retained payload without leaving a stale point footprint.
            vec3 grown_positions[3] = {{-.65f, 0, .2f}, {0, .65f, .2f}, {.65f, 0, .2f}};
            DvzColor grown_colors[3] = {
                {255, 0, 0, 192}, {255, 0, 0, 192}, {255, 0, 0, 192}};
            float grown_sizes[3] = {8, 8, 8};
            DvzVisualDataUpdate replacements[] = {
                {"position", grown_positions, 3},
                {"color", grown_colors, 3},
                {"diameter_px", grown_sizes, 3},
            };
            AT(dvz_visual_set_data_many(points[0], replacements, 3) == DVZ_OK);
            AT(_point_wboit_capture(view, pixels));
            const uint32_t right = 4 * (32 * 64 + 53);
            AT(pixels[right] > 80);
            for (uint32_t i = 0; i < 3; i++)
                replacements[i].item_count = 1;
            AT(dvz_visual_set_data_many(points[0], replacements, 3) == DVZ_OK);
            AT(_point_wboit_capture(view, pixels));
            AT(pixels[right] == 0);
            AT(pixels[moved] > 80);

            // An opaque surface in front hides the transparent point without changing its alpha.
            DvzVisual* mesh = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
            vec3 quad[6] = {{-1, -1, -0.2f}, {1, -1, -0.2f}, {-1, 1, -0.2f},
                            {-1, 1, -0.2f}, {1, -1, -0.2f}, {1, 1, -0.2f}};
            DvzColor green[6] = {{0, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 0, 255},
                                {0, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 0, 255}};
            AT(dvz_visual_set_data(mesh, "position", quad, 6) == DVZ_OK);
            AT(dvz_visual_set_data(mesh, "color", green, 6) == DVZ_OK);
            AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);
            AT(_point_wboit_capture(view, pixels));
            AT(pixels[center] <= 1 && pixels[center + 2] <= 1);
            AT(pixels[center + 1] > 250);
            AT(pixels[moved] <= 1 && pixels[moved + 1] > 250);
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
        }
    }
    dvz_gpu_ctx_destroy(gpu_ctx);
    return 0;
}
