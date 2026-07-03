/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene DPI tests                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <string.h>

#include "_assertions.h"
#include "../../drp2/_stream.h"
#include "frame_plan/frame_plan.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Verify DPI scaling reaches DRP2 viewport/scissor, shader viewport, and point sizes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_dpi_physical_viewport_and_screen_scale(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 400, 300, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.25f, 0.0f, 0.5f, 1.0f});
    AT(panel != NULL);

    DvzVisual* point = dvz_point(scene, 0);
    AT(point != NULL);
    vec3 pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 255, 255, 255}};
    float size[1] = {8.0f};
    AT(dvz_visual_set_data(point, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(point, "color", color, 1) == 0);
    AT(dvz_visual_set_data(point, "size", size, 1) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    vec3 image_pos[1] = {{10.0f, 20.0f, 0.0f}};
    vec2 image_extent[1] = {{30.0f, 40.0f}};
    vec2 image_anchor[1] = {{-1.0f, -1.0f}};
    uint8_t image_pixels[4 * 4 * 4] = {0};
    AT(dvz_visual_set_data(image, "position_px", image_pos, 1) == 0);
    AT(dvz_visual_set_data(image, "extent_px", image_extent, 1) == 0);
    AT(dvz_visual_set_data(image, "anchor", image_anchor, 1) == 0);
    AT(dvz_visual_set_texture_rgba8(image, (const uint8_t*)image_pixels, 4, 4, 4u * 4u * 4u) == 0);
    AT(dvz_panel_add_visual(
           panel, image,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 800;
    cfg.target_height = 600;
    cfg.device_scale_x = 2.0f;
    cfg.device_scale_y = 2.0f;
    cfg.user_scale = 1.5f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_viewport = false;
    bool found_scissor = false;
    bool found_viewport_uniform = false;
    bool found_size_upload = false;
    bool found_image_position_upload = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
        {
            AC(cmd->u.set_viewport.viewport[0], 200.0f, 1e-6f);
            AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.set_viewport.viewport[2], 400.0f, 1e-6f);
            AC(cmd->u.set_viewport.viewport[3], 600.0f, 1e-6f);
            found_viewport = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
        {
            AC(cmd->u.set_scissor.scissor[0], 200.0f, 1e-6f);
            AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
            AC(cmd->u.set_scissor.scissor[2], 400.0f, 1e-6f);
            AC(cmd->u.set_scissor.scissor[3], 600.0f, 1e-6f);
            found_scissor = true;
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size == sizeof(DvzSceneViewportUniform))
        {
            const DvzSceneViewportUniform* viewport =
                (const DvzSceneViewportUniform*)cmd->u.write_buffer.data_raw;
            ANN(viewport);
            if (fabsf(viewport->width - 400.0f) <= 1e-6f &&
                fabsf(viewport->height - 600.0f) <= 1e-6f)
            {
                AC(viewport->x, 200.0f, 1e-6f);
                AC(viewport->y, 0.0f, 1e-6f);
                found_viewport_uniform = true;
            }
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size == sizeof(float))
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            if (label != NULL && strstr(label, "_size") != NULL)
            {
                const float* uploaded = (const float*)cmd->u.write_buffer.data_raw;
                ANN(uploaded);
                AC(uploaded[0], 24.0f, 1e-6f);
                found_size_upload = true;
            }
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size == 18 * sizeof(float))
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            if (label != NULL && strstr(label, "_position") != NULL)
            {
                const float* uploaded = (const float*)cmd->u.write_buffer.data_raw;
                ANN(uploaded);
                const float expected[18] = {
                    20.0f, 40.0f, 0.0f, 20.0f, 120.0f, 0.0f, 80.0f, 40.0f, 0.0f,
                    80.0f, 40.0f, 0.0f, 20.0f, 120.0f, 0.0f, 80.0f, 120.0f, 0.0f,
                };
                bool matches = true;
                for (uint32_t j = 0; j < 18; j++)
                    matches = matches && fabsf(uploaded[j] - expected[j]) < 1e-6f;
                found_image_position_upload = found_image_position_upload || matches;
            }
        }
    }

    AT(found_viewport);
    AT(found_scissor);
    AT(found_viewport_uniform);
    AT(found_size_upload);
    AT(found_image_position_upload);

    _test_scene_stream_destroy(stream);

    cfg.user_scale = 2.0f;
    DvzDiagnosticReport report2;
    dvz_diagnostic_report_init(&report2);
    stream = _test_scene_emit_stream_ex(figure, &caps, &report2, &cfg);
    AT(dvz_diagnostic_report_count(&report2) == 0);
    ANN(stream);

    bool found_rescaled_size_upload = false;
    count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER ||
            cmd->u.write_buffer.size != sizeof(float))
            continue;
        const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
        if (label == NULL || strstr(label, "_size") == NULL)
            continue;
        const float* uploaded = (const float*)cmd->u.write_buffer.data_raw;
        ANN(uploaded);
        if (fabsf(uploaded[0] - 32.0f) < 1e-6f)
            found_rescaled_size_upload = true;
    }
    AT(found_rescaled_size_upload);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify user scale affects built-in marker edge widths in material uploads.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_dpi_user_scale_marker_edge_width(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 400, 300, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel_full(figure);
    AT(panel != NULL);

    DvzVisual* marker = dvz_marker(scene, 0);
    AT(marker != NULL);

    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    style.stroke_width_px = 2.75f;
    AT(dvz_marker_set_style(marker, &style) == 0);

    vec3 pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 255, 255, 255}};
    float diameter_px[1] = {24.0f};
    float angle[1] = {0.0f};
    uint32_t symbol[1] = {DVZ_SYMBOL_DISC};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = pos, .item_count = 1},
        {.attr_name = "color", .data = color, .item_count = 1},
        {.attr_name = "diameter_px", .data = diameter_px, .item_count = 1},
        {.attr_name = "angle", .data = angle, .item_count = 1},
        {.attr_name = "symbol", .data = symbol, .item_count = 1},
    };
    AT(dvz_visual_set_data_many(marker, updates, DVZ_ARRAY_COUNT(updates)) == 0);
    AT(dvz_panel_add_visual(panel, marker, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 400;
    cfg.target_height = 300;
    cfg.user_scale = 1.0f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    _test_scene_stream_destroy(stream);

    cfg.user_scale = 2.0f;
    dvz_diagnostic_report_init(&report);
    stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_material_params = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER ||
            cmd->u.write_buffer.size != sizeof(DvzSceneMaterialParams))
        {
            continue;
        }

        const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
        if (label == NULL || strstr(label, "material_params") == NULL)
            continue;

        const DvzSceneMaterialParams* params =
            (const DvzSceneMaterialParams*)cmd->u.write_buffer.data_raw;
        ANN(params);
        AC(params->params[0], style.stroke_width_px * cfg.user_scale, 1e-6f);
        found_material_params = true;
    }
    AT(found_material_params);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify user scale affects generated axis segment widths.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_dpi_user_scale_axis_segment_width(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel_full(figure);
    AT(panel != NULL);

    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    AT(axis != NULL);
    DvzAxisStyle style = dvz_axis_style();
    style.spine_width = 4.0f;
    style.show_major_ticks = false;
    style.show_minor_ticks = false;
    style.show_grid = false;
    AT(dvz_axis_set_style(axis, &style));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.supports_color_blending = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 800;
    cfg.target_height = 600;
    cfg.user_scale = 1.0f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    _test_scene_stream_destroy(stream);

    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(axis->visual, "position", &position_view) == 0);
    AT(position_view.item_count >= 6);
    const float* positions = (const float*)position_view.data;
    const float thickness0 = fabsf(positions[7] - positions[1]);
    AC(thickness0, 2.0f * style.spine_width / 600.0f, 1e-6f);

    cfg.user_scale = 2.0f;
    dvz_diagnostic_report_init(&report);
    stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    _test_scene_stream_destroy(stream);

    position_view = (DvzVisualDataView){0};
    AT(dvz_visual_data(axis->visual, "position", &position_view) == 0);
    AT(position_view.item_count >= 6);
    positions = (const float*)position_view.data;
    const float thickness1 = fabsf(positions[7] - positions[1]);
    AC(thickness1, 2.0f * style.spine_width * cfg.user_scale / 600.0f, 1e-6f);
    AC(thickness1, 2.0f * thickness0, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify user scale affects left-axis tick/title text placement gaps.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_dpi_user_scale_axis_text_gap(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel_full(figure);
    AT(panel != NULL);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, -1.0, +1.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, +1.0) == 0);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    AT(y_axis != NULL);
    AT(dvz_axis_set_label(y_axis, "amplitude"));
    DvzAxisStyle style = dvz_axis_style();
    style.tick_gap_px = 8.0f;
    style.label_gap_px = 12.0f;
    style.tick_size_px = 16.0f;
    AT(dvz_axis_set_style(y_axis, &style));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.supports_color_blending = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 800;
    cfg.target_height = 600;

    float offsets[2] = {0};
    for (uint32_t pass = 0; pass < 2; pass++)
    {
        cfg.user_scale = pass == 0 ? 1.0f : 2.0f;
        DvzDiagnosticReport report;
        dvz_diagnostic_report_init(&report);
        DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
        AT(dvz_diagnostic_report_count(&report) == 0);
        ANN(stream);
        _test_scene_stream_destroy(stream);

        DvzRect plot = {0};
        AT(dvz_panel_plot_rect_px(panel, &plot));
        bool found_label = false;
        for (uint32_t i = 0; i < y_axis->text_count; i++)
        {
            if (strcmp(y_axis->text_labels[i], "amplitude") != 0)
                continue;
            offsets[pass] = plot.x - y_axis->text_positions[i][0];
            found_label = true;
            break;
        }
        AT(found_label);
    }

    const float expected = style.tick_gap_px + 2.25f * style.tick_size_px + style.label_gap_px;
    AC(offsets[0], expected, 1e-4f);
    AC(offsets[1], 2.0f * expected, 1e-4f);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify user scale affects resolved panel reserves around the plot area.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_dpi_user_scale_panel_margin(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel_full(figure);
    AT(panel != NULL);

    AT(dvz_panel_set_reserve(
        panel, &(DvzPanelReserve){.left_px = 40.0f, .right_px = 20.0f, .top_px = 30.0f,
                                  .bottom_px = 10.0f}));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.supports_color_blending = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 800;
    cfg.target_height = 600;
    cfg.user_scale = 2.0f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    _test_scene_stream_destroy(stream);

    DvzRect plot = {0};
    AT(dvz_panel_plot_rect_px(panel, &plot));
    AC(plot.x, 80.0f, 1e-4f);
    AC(plot.y, 60.0f, 1e-4f);
    AC(plot.width, 680.0f, 1e-4f);
    AC(plot.height, 520.0f, 1e-4f);

    DvzPanelDesc plot_desc = _scene_panel_plot_desc(panel);
    AC(plot_desc.x, 0.10f, 1e-6f);
    AC(plot_desc.y, 0.10f, 1e-6f);
    AC(plot_desc.width, 0.85f, 1e-6f);
    AC(plot_desc.height, 520.0f / 600.0f, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify user scale applies to manual logical-pixel panel reserves.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_dpi_user_scale_pixel_reserve(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel_full(figure);
    AT(panel != NULL);

    AT(dvz_panel_set_reserve(
        panel, &(DvzPanelReserve){.left_px = 40.0f, .right_px = 20.0f, .bottom_px = 36.0f,
                                        .top_px = 24.0f}));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.supports_color_blending = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 800;
    cfg.target_height = 600;
    cfg.user_scale = 2.0f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    _test_scene_stream_destroy(stream);

    DvzRect plot = {0};
    AT(dvz_panel_plot_rect_px(panel, &plot));
    AC(plot.x, 80.0f, 1e-4f);
    AC(plot.y, 48.0f, 1e-4f);
    AC(plot.width, 680.0f, 1e-4f);
    AC(plot.height, 480.0f, 1e-4f);

    DvzPanelDesc plot_desc = _scene_panel_plot_desc(panel);
    AC(plot_desc.x, 0.10f, 1e-6f);
    AC(plot_desc.y, 0.08f, 1e-6f);
    AC(plot_desc.width, 0.85f, 1e-6f);
    AC(plot_desc.height, 0.80f, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_dpi(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene,dpi";
    TST_CASE(test_scene_dpi_physical_viewport_and_screen_scale);
    TST_CASE(test_scene_dpi_user_scale_marker_edge_width);
    TST_CASE(test_scene_dpi_user_scale_axis_segment_width);
    TST_CASE(test_scene_dpi_user_scale_axis_text_gap);
    TST_CASE(test_scene_dpi_user_scale_panel_margin);
    TST_CASE(test_scene_dpi_user_scale_pixel_reserve);
    return 0;
}
