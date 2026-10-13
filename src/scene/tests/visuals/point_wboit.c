/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Point accumulation contracts                                                                 */
/*************************************************************************************************/

#include "_visual_pipeline_internal.h"
#include "common.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"



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
        AT(strstr(shader.fragment_glsl, "applySceneOcclusion(outColor)") != NULL);
        AT(strstr(shader.fragment_wgsl, "surface.a <= 0.0") != NULL);
        if ((variant & 1) != 0)
        {
            AT(strstr(shader.fragment_glsl, "pointStyleSurface()") != NULL);
            AT(strstr(shader.fragment_wgsl, "point_style_surface(input.color, dist)") != NULL);
            AT(strstr(
                   shader.fragment_wgsl,
                   "select(edge_color, mix(linear_color, edge_color, stroke_mask), filled)") !=
               NULL);
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
            AT(pipeline.has_raster_state);
            AT(pipeline.cull_mode == DVZ_CULL_MODE_NONE);
            AT(pipeline.front_face == DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
            AT(pipeline.topology == (backend == 0 ? DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST
                                                  : DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST));
        }
        visual.has_item_state = true;
        AT(!_scene_visual_shader_desc_resolve(&visual, false, true, "_wgsl", &shader));
    }
    AT(_scene_visual_alpha_eligibility_error(DVZ_SCENE_VISUAL_DESC_PIXEL, DVZ_ALPHA_WBOIT) !=
       NULL);
    AT(_scene_visual_alpha_eligibility_error(DVZ_SCENE_VISUAL_DESC_MARKER, DVZ_ALPHA_WBOIT) !=
       NULL);
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
    return dvz_canvas_capture_rgba_into(dvz_view_canvas(view), 64, 64, pixels, 64 * 64 * 4) ==
           DVZ_OK;
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
            DvzVisual* transparent_mesh = dvz_mesh(scene, 0);
            vec3 mesh_positions[6] = {{-.3f, -.3f, .5f}, {.3f, -.3f, .5f}, {-.3f, .3f, .5f},
                                      {-.3f, .3f, .5f},  {.3f, -.3f, .5f}, {.3f, .3f, .5f}};
            DvzColor mesh_colors[6] = {{0, 255, 0, 32}, {0, 255, 0, 32}, {0, 255, 0, 32},
                                       {0, 255, 0, 32}, {0, 255, 0, 32}, {0, 255, 0, 32}};
            AT(dvz_visual_set_data(transparent_mesh, "position", mesh_positions, 6) == DVZ_OK);
            AT(dvz_visual_set_data(transparent_mesh, "color", mesh_colors, 6) == DVZ_OK);
            AT(dvz_visual_set_alpha_mode(transparent_mesh, DVZ_ALPHA_WBOIT) == DVZ_OK);
            AT(dvz_panel_add_visual(panel, points[reverse], NULL) == DVZ_OK);
            AT(dvz_panel_add_visual(panel, transparent_mesh, NULL) == DVZ_OK);
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
            DvzColor grown_colors[3] = {{255, 0, 0, 192}, {255, 0, 0, 192}, {255, 0, 0, 192}};
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
                            {-1, 1, -0.2f},  {1, -1, -0.2f}, {1, 1, -0.2f}};
            DvzColor green[6] = {{0, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 0, 255},
                                 {0, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 0, 255}};
            AT(dvz_visual_set_data(mesh, "position", quad, 6) == DVZ_OK);
            AT(dvz_visual_set_data(mesh, "color", green, 6) == DVZ_OK);
            AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);
            AT(_point_wboit_capture(view, pixels));
            AT(pixels[center] <= 1 && pixels[center + 2] <= 1);
            AT(pixels[center + 1] > 250);
            AT(pixels[moved] <= 1 && pixels[moved + 1] > 250);
            // Explicit hidden-surface attenuation must survive both style and cue variants.
            AT(dvz_visual_set_visible(points[0], false) == DVZ_OK);
            AT(dvz_visual_set_visible(transparent_mesh, false) == DVZ_OK);
            DvzCameraDesc camera = dvz_camera_desc();
            camera.view.eye[2] = 3;
            AT(dvz_panel_set_camera_desc(panel, &camera) == DVZ_OK);
            vec3 behind = {0, 0, -.5f};
            AT(dvz_visual_set_data(points[1], "position", &behind, 1) == DVZ_OK);
            AT(dvz_visual_set_depth_test(points[1], false) == DVZ_OK);
            AT(_point_wboit_capture(view, hidden));
            AT(hidden[center + 2] > 60);
            AT(dvz_visual_set_scene_occluder(mesh, true) == DVZ_OK);
            AT(dvz_visual_set_scene_occluded(points[1], true) == DVZ_OK);
            DvzSceneOcclusionDesc occlusion = {
                DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
                .enabled = true,
                .depth_bias = .0001f,
                .soft_edge = .01f,
                .hidden_alpha = .1f,
            };
            AT(dvz_panel_set_scene_occlusion(panel, &occlusion) == DVZ_OK);
            AT(_point_wboit_capture(view, pixels));
            AT(pixels[center + 2] + 20 < hidden[center + 2]);
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
        }
    }
    // Match authored CCW fronts across opaque and two-sided lit accumulation shaders.
    for (uint32_t reversed = 0; reversed < 2; reversed++)
    {
        for (uint32_t accumulation = 0; accumulation < 2; accumulation++)
        {
            DvzScene* scene = dvz_scene();
            DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
            DvzPanel* panel = dvz_panel_full(figure);
            dvz_panel_set_background_color(panel, dvz_color_rgba(0, 0, 0, 255));
            DvzCameraDesc camera = dvz_camera_desc();
            camera.view.eye[2] = 3;
            AT(dvz_panel_set_camera_desc(panel, &camera) == DVZ_OK);
            DvzLightDesc light_desc = dvz_light_desc(DVZ_LIGHT_DIRECTIONAL);
            glm_vec3_copy((vec3){0, 0, 1}, light_desc.direction);
            light_desc.intensity = 1;
            DvzLight* light = dvz_light(scene, &light_desc);
            DvzLight* lights[] = {light};
            AT(dvz_panel_set_lights(panel, lights, 1) == DVZ_OK);
            DvzVisual* mesh = dvz_mesh(scene, 0);
            vec3 positions[3] = {{-.8f, -.7f, 0}, {.8f, -.7f, 0}, {0, .8f, 0}};
            if (reversed != 0)
            {
                vec3 temporary = {0};
                glm_vec3_copy(positions[0], temporary);
                glm_vec3_copy(positions[2], positions[0]);
                glm_vec3_copy(temporary, positions[2]);
            }
            vec3 normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
            DvzColor colors[3] = {{255, 255, 255, 255}, {255, 255, 255, 255},
                                  {255, 255, 255, 255}};
            DvzVisualDataUpdate updates[] = {
                {"position", positions, 3}, {"normal", normals, 3}, {"color", colors, 3}};
            AT(dvz_visual_set_data_many(mesh, updates, 3) == DVZ_OK);
            DvzMaterialDesc material = dvz_phong_material_desc();
            material.phong.ambient = 0;
            material.phong.diffuse = 1;
            material.phong.specular = 0;
            material.alpha_mode = accumulation != 0 ? DVZ_ALPHA_WBOIT : DVZ_ALPHA_OPAQUE;
            AT(dvz_visual_set_material(mesh, &material) == DVZ_OK);
            AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);
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
            uint32_t center = 4 * (32 * 64 + 32);
            bool illuminated = accumulation == 0 || reversed == 0;
            for (uint32_t channel = 0; channel < 3; channel++)
                AT(illuminated ? pixels[center + channel] > 250
                               : pixels[center + channel] < 5);
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
        }
    }
    dvz_gpu_ctx_destroy(gpu_ctx);
    return 0;
}



/**
 * Verify WGSL accumulation packets retain instanced point layouts and current payload extents.
 *
 * @param suite active test context
 * @param item active test item
 * @return 0 on success
 */
int test_scene_point_wboit_wgsl_packets(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    for (uint32_t variant = 0; variant < 4; variant++)
    {
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
        DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){.25f, .125f, .5f, .75f});
        DvzVisual* point = dvz_point(scene, 0);
        vec3 positions[3] = {{-.5f, -.5f, .2f}, {0, 0, .2f}, {.5f, .5f, .2f}};
        DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 0}, {0, 0, 255, 192}};
        float sizes[3] = {8, 12, 16};
        DvzVisualDataUpdate updates[] = {
            {"position", positions, 3}, {"color", colors, 3}, {"diameter_px", sizes, 3}};
        AT(dvz_visual_set_data_many(point, updates, 3) == DVZ_OK);
        AT(dvz_visual_set_alpha_mode(point, DVZ_ALPHA_WBOIT) == DVZ_OK);
        if ((variant & 1) != 0)
        {
            DvzPointStyleDesc style = dvz_point_style_desc();
            style.aspect = DVZ_SHAPE_ASPECT_STROKE;
            style.stroke_width_px = 2;
            style.edge_color = (DvzColor){255, 255, 0, 96};
            AT(dvz_point_set_style(point, &style) == DVZ_OK);
        }
        if ((variant & 2) != 0)
        {
            DvzDepthCueDesc cue = dvz_depth_cue_desc();
            AT(dvz_visual_set_depth_cue(point, &cue) == DVZ_OK);
        }
        AT(dvz_panel_add_visual(panel, point, NULL) == DVZ_OK);
        DvzCapabilitySnapshot caps = dvz_capability_snapshot();
        caps.shader_format_wgsl = true;
        caps.shader_format_glsl = false;
        caps.max_color_attachments = 2;
        caps.render_target_format_rgba16float = true;
        caps.render_target_format_r16float = true;
        caps.supports_color_blending = true;
        caps.supports_render_target_sampling = true;
        caps.max_vertex_buffers = 16;
        caps.max_bind_groups = 4;
        caps.max_buffer_size = 256 * 1024 * 1024;
        DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
        cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
        uint64_t point_shader = 0;
        uint64_t point_pipeline = 0;
        for (uint32_t replacement = 0; replacement < 3; replacement++)
        {
            uint32_t count = replacement == 1 ? 1 : 3;
            for (uint32_t i = 0; i < 3; i++)
                updates[i].item_count = count;
            AT(dvz_visual_set_data_many(point, updates, 3) == DVZ_OK);
            DvzDiagnosticReport report;
            dvz_diagnostic_report_init(&report);
            DvzDrp2CommandStream* stream =
                _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
            AT(dvz_diagnostic_report_count(&report) == 0);
            ANN(stream);
            uint64_t current_pipeline = 0;
            bool found_accum = false;
            bool found_draw = false;
            for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
            {
                const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
                if (command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE &&
                    strcmp(command->u.create_shader_module.stage, "FRAGMENT") == 0 &&
                    strcmp(command->u.create_shader_module.builtin_family, "scene.point") == 0)
                {
                    point_shader = command->u.create_shader_module.id;
                    AT(strstr(command->u.create_shader_module.code, "scene_wboit_weight") != NULL);
                    AT(strstr(command->u.create_shader_module.code, "point_disc_coverage") !=
                       NULL);
                }
                if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE &&
                    command->u.create_render_pipeline.fragment_shader_module_id == point_shader)
                {
                    point_pipeline = command->u.create_render_pipeline.id;
                    AT(command->u.create_render_pipeline.color_target_count == 2);
                    AT(command->u.create_render_pipeline.color_targets[0].format ==
                       DVZ_FORMAT_R16G16B16A16_SFLOAT);
                    AT(command->u.create_render_pipeline.color_targets[1].format ==
                       DVZ_FORMAT_R16_SFLOAT);
                    for (uint32_t target = 0; target < 2; target++)
                    {
                        const DvzDrp2ColorTarget* blend =
                            &command->u.create_render_pipeline.color_targets[target];
                        AT(blend->blend_enabled);
                        AT(blend->src_color_blend_factor == DVZ_BLEND_FACTOR_ONE);
                        AT(blend->dst_color_blend_factor == DVZ_BLEND_FACTOR_ONE);
                        AT(blend->color_blend_op == DVZ_BLEND_OP_ADD);
                    }
                    AT(command->u.create_render_pipeline.has_depth_attachment);
                    AT(!command->u.create_render_pipeline.depth_write_enabled);
                    AT(command->u.create_render_pipeline.topology ==
                       DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                    for (uint32_t binding = 0; binding < 3; binding++)
                        AT(command->u.create_render_pipeline.binding_step_modes[binding] ==
                           DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
                }
                if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
                    command->u.begin_render_pass.color_attachment_count == 2)
                {
                    found_accum = true;
                    AT(command->u.begin_render_pass.has_depth_attachment);
                }
                if (command->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
                    current_pipeline = command->u.set_pipeline.pipeline_id;
                if (command->type == DVZ_DRP2_COMMAND_DRAW && current_pipeline == point_pipeline)
                {
                    found_draw = true;
                    AT(command->u.draw.vertex_count == 6);
                    AT(command->u.draw.instance_count == count);
                }
            }
            AT(point_shader != 0);
            AT(point_pipeline != 0);
            AT(found_accum);
            AT(found_draw);
            _test_scene_stream_destroy(stream);
        }
        dvz_scene_destroy(scene);
    }
    return 0;
}
