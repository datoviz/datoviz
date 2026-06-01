/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene interaction graph tests                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_graph_utils.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_panel_full_helper(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 32, 0);
    DvzPanel* panel = dvz_panel_full(figure);

    ANN(panel);
    AT(panel->desc.x == 0.0f);
    AT(panel->desc.y == 0.0f);
    AT(panel->desc.width == 1.0f);
    AT(panel->desc.height == 1.0f);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_grid_resolve_weights_fixed_and_spans(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 400, 200, 0);
    DvzGrid* grid = dvz_figure_grid(figure, 2, 3);
    ANN(grid);

    AT(dvz_grid_set_margins(
        grid, &(DvzPanelReserve){
                  .left_px = 40.0f, .right_px = 20.0f, .top_px = 10.0f,
                  .bottom_px = 30.0f}));
    AT(dvz_grid_set_gutter(grid, 10.0f, 20.0f));
    AT(dvz_grid_col_size(grid, 0, DVZ_GRID_SIZE_WEIGHT, 1.0f));
    AT(dvz_grid_col_size(grid, 1, DVZ_GRID_SIZE_WEIGHT, 2.0f));
    AT(dvz_grid_col_size(grid, 2, DVZ_GRID_SIZE_FIXED_PX, 60.0f));

    DvzPanelDesc main = {0};
    AT(dvz_grid_resolve(
        grid, 400, 200, (DvzGridCell){.row = 0, .col = 0, .row_span = 1, .col_span = 2},
        &main));
    AC(main.x, 0.10f, 1e-6f);
    AC(main.y, 0.05f, 1e-6f);
    AC(main.width, 0.675f, 1e-6f);
    AC(main.height, 0.35f, 1e-6f);

    DvzPanelDesc side = {0};
    AT(dvz_grid_resolve(
        grid, 400, 200, (DvzGridCell){.row = 0, .col = 2, .row_span = 2, .col_span = 1},
        &side));
    AC(side.x, 0.80f, 1e-6f);
    AC(side.y, 0.05f, 1e-6f);
    AC(side.width, 0.15f, 1e-6f);
    AC(side.height, 0.80f, 1e-6f);

    DvzPanelDesc resized = {0};
    AT(dvz_grid_resolve(
        grid, 800, 200, (DvzGridCell){.row = 0, .col = 2, .row_span = 2, .col_span = 1},
        &resized));
    AC(resized.x, 0.90f, 1e-6f);
    AC(resized.width, 0.075f, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_grid_resolve_rejects_invalid_inputs(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzGrid* grid = dvz_figure_grid(figure, 2, 2);
    ANN(grid);

    AT(!dvz_figure_grid(figure, 0, 2));
    AT(!dvz_figure_grid(figure, 2, 0));
    AT(!dvz_grid_col_size(grid, 0, DVZ_GRID_SIZE_WEIGHT, 0.0f));
    AT(!dvz_grid_row_size(grid, 0, DVZ_GRID_SIZE_FIXED_PX, -1.0f));
    AT(!dvz_grid_set_gutter(grid, -1.0f, 0.0f));
    AT(!dvz_grid_set_margins(grid, &(DvzPanelReserve){.left_px = -1.0f}));

    DvzPanelDesc desc = {0};
    AT(!dvz_grid_resolve(
        grid, 64, 64, (DvzGridCell){.row = 1, .col = 1, .row_span = 2, .col_span = 1},
        &desc));
    AT(!dvz_grid_resolve(
        grid, 0, 64, (DvzGridCell){.row = 0, .col = 0, .row_span = 1, .col_span = 1},
        &desc));

    AT(dvz_grid_set_margins(
        grid, &(DvzPanelReserve){.left_px = 40.0f, .right_px = 40.0f}));
    AT(!dvz_grid_resolve(
        grid, 64, 64, (DvzGridCell){.row = 0, .col = 0, .row_span = 1, .col_span = 1},
        &desc));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_grid_panel_recomputes_before_emit(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    DvzGrid* grid = dvz_figure_grid(figure, 1, 2);
    ANN(grid);
    DvzPanel* left = dvz_grid_panel(grid, 0, 0);
    DvzPanel* right = dvz_grid_panel(grid, 0, 1);
    ANN(left);
    ANN(right);

    AT(dvz_grid_col_size(grid, 0, DVZ_GRID_SIZE_FIXED_PX, 60.0f));

    float pos[3] = {0.0f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float sz = 5.0f;
    DvzVisual* vl = dvz_point(scene, 0);
    DvzVisual* vr = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vl, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(vl, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vl, "size", &sz, 1) == 0);
    AT(dvz_visual_set_data(vr, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(vr, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vr, "size", &sz, 1) == 0);
    AT(dvz_panel_add_visual(left, vl, NULL) == 0);
    AT(dvz_panel_add_visual(right, vr, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 200;
    cfg.target_height = 100;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t viewport_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type != DVZ_DRP2_COMMAND_SET_VIEWPORT)
            continue;
        if (viewport_count == 0)
        {
            AC(cmd->u.set_viewport.viewport[0], 0.0f, 1e-6f);
            AC(cmd->u.set_viewport.viewport[2], 60.0f, 1e-5f);
        }
        else if (viewport_count == 1)
        {
            AC(cmd->u.set_viewport.viewport[0], 60.0f, 1e-5f);
            AC(cmd->u.set_viewport.viewport[2], 140.0f, 1e-5f);
        }
        viewport_count++;
    }
    AT(viewport_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_grid_panel_tracks_figure_resize(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    DvzGrid* grid = dvz_figure_grid(figure, 1, 2);
    ANN(grid);
    AT(dvz_grid_set_gutter(grid, 10.0f, 0.0f));
    AT(dvz_grid_col_size(grid, 1, DVZ_GRID_SIZE_FIXED_PX, 60.0f));

    DvzPanel* data = dvz_grid_panel(grid, 0, 0);
    DvzPanel* colorbar = dvz_grid_panel(grid, 0, 1);
    ANN(data);
    ANN(colorbar);

    AC(data->desc.x, 0.0f, 1e-6f);
    AC(data->desc.width, 0.65f, 1e-6f);
    AC(colorbar->desc.x, 0.70f, 1e-6f);
    AC(colorbar->desc.width, 0.30f, 1e-6f);

    dvz_figure_resize(figure, 400, 100);
    AC(data->desc.x, 0.0f, 1e-6f);
    AC(data->desc.width, 0.825f, 1e-6f);
    AC(colorbar->desc.x, 0.85f, 1e-6f);
    AC(colorbar->desc.width, 0.15f, 1e-6f);

    AT(dvz_panel_set_desc(data, (DvzPanelDesc){.x = 0.1f, .y = 0.1f, .width = 0.8f,
                                               .height = 0.8f}));
    dvz_figure_resize(figure, 500, 100);
    AC(data->desc.x, 0.1f, 1e-6f);
    AC(data->desc.width, 0.8f, 1e-6f);
    AC(colorbar->desc.x, 0.88f, 1e-6f);
    AC(colorbar->desc.width, 0.12f, 1e-6f);

    AT(!dvz_panel_set_desc(colorbar, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 0.0f,
                                                    .height = 1.0f}));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_z_layer_orders_emit(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    /* Two point visuals on one panel: behind=3 verts (z=-1), front=5 verts (z=+1).
     * Add front first, behind second, so insertion order ≠ z order. After phase 1
     * both visuals draw inside one render pass; the behind visual (z=-1) must draw
     * before the front visual (z=+1). */
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    float pos5[5 * 3] = {0};
    float pos3[3 * 3] = {0};
    DvzColor col[5] = {0};
    float sz[5] = {0};

    DvzVisual* v_front  = dvz_point(scene, 0);  /* z=+1, 5 verts */
    DvzVisual* v_behind = dvz_point(scene, 0);  /* z=-1, 3 verts */

    AT(dvz_visual_set_data(v_front, "position", pos5, 5) == 0);
    AT(dvz_visual_set_data(v_front, "color",    col,  5) == 0);
    AT(dvz_visual_set_data(v_front, "size",     sz,   5) == 0);
    AT(dvz_visual_set_data(v_behind, "position", pos3, 3) == 0);
    AT(dvz_visual_set_data(v_behind, "color",    col,  3) == 0);
    AT(dvz_visual_set_data(v_behind, "size",     sz,   3) == 0);

    AT(dvz_panel_add_visual(panel, v_front,  &(DvzVisualAttachDesc){.z_layer = +1}) == 0);
    AT(dvz_panel_add_visual(panel, v_behind, &(DvzVisualAttachDesc){.z_layer = -1}) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    char* json = dvz_drp2_stream_json(stream, "z_layer_order");
    ANN(json);

    /* Both draws appear in the single render pass (pass_id 10001).
     * The behind visual (3 verts, z=-1) must appear before the front visual (5 verts, z=+1). */
    const char* draw3 = strstr(json, "\"cmd\": \"Draw\", \"pass_id\": 10001, \"vertex_count\": 3");
    const char* draw5 = strstr(json, "\"cmd\": \"Draw\", \"pass_id\": 10001, \"vertex_count\": 5");
    AT(draw3 != NULL);
    AT(draw5 != NULL);
    AT(draw3 < draw5);  /* behind drawn first */

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_background_color_creates_fixed_quad(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    /* Initially no visuals. */
    AT(panel->visual_count == 0);
    AT(panel->background_visual == NULL);

    /* First call: creates a hidden background visual at z_layer=-1, FIXED. */
    dvz_panel_set_background_color(panel, 0.1f, 0.2f, 0.3f, 1.0f);
    AT(panel->visual_count == 1);
    ANN(panel->background_visual);
    AT(panel->visuals[0].visual == panel->background_visual);
    AT(panel->visuals[0].z_layer == -1);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);
    AT(panel->background_type == DVZ_PANEL_BACKGROUND_COLOR);

    /* Second call with a different color: updates in place, no new visual. */
    DvzVisual* before = panel->background_visual;
    dvz_panel_set_background_color(panel, 0.9f, 0.8f, 0.7f, 1.0f);
    AT(panel->visual_count == 1);
    AT(panel->background_visual == before);

    /* A regular visual added afterwards has default attach (z=0, APPLY) and lands
     * in front of the background per stable z-sort. */
    float pos[3 * 3] = {0};
    DvzColor col[3]  = {0};
    float sz[3]      = {0};
    DvzVisual* v = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v, "size", sz, 3) == 0);
    AT(dvz_panel_add_visual(panel, v, NULL) == 0);
    AT(panel->visual_count == 2);
    AT(panel->visuals[1].z_layer == 0);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_APPLY);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_background_descriptor_gradient_and_image(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    DvzPanelBackgroundDesc gradient = {
        .type = DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT,
        .gradient = {
            .start = {0.0f, 0.0f},
            .end = {1.0f, 0.0f},
            .color0 = {1.0f, 0.0f, 0.0f, 1.0f},
            .color1 = {0.0f, 0.0f, 1.0f, 1.0f},
        },
    };
    AT(dvz_panel_set_background(panel, &gradient));
    AT(panel->visual_count == 1);
    ANN(panel->background_visual);
    AT(panel->background_type == DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT);
    AT(panel->background_visual->type == DVZ_VISUAL_TYPE_PRIMITIVE);
    AT(panel->visuals[0].z_layer == -1);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);

    int color_idx = _attr_index(panel->background_visual, "color");
    AT(color_idx >= 0);
    const DvzColor* colors = (const DvzColor*)panel->background_visual->attrs[color_idx].data;
    ANN(colors);
    AT(colors[0].r == 255);
    AT(colors[0].b == 0);
    AT(colors[1].r == 255);
    AT(colors[1].b == 0);
    AT(colors[2].r == 0);
    AT(colors[2].b == 255);
    AT(colors[3].r == 0);
    AT(colors[3].b == 255);

    DvzVisual* gradient_visual = panel->background_visual;
    gradient.gradient.end[0] = 0.0f;
    gradient.gradient.end[1] = 1.0f;
    AT(dvz_panel_set_background(panel, &gradient));
    AT(panel->visual_count == 1);
    AT(panel->background_visual == gradient_visual);

    uint8_t pixels[2 * 2 * 4] = {
        255, 0, 0, 255, 0, 255, 0, 255,
        0, 0, 255, 255, 255, 255, 255, 255,
    };
    DvzPanelBackgroundDesc image = {
        .type = DVZ_PANEL_BACKGROUND_IMAGE,
        .image = {.rgba = pixels, .width = 2, .height = 2},
    };
    AT(dvz_panel_set_background(panel, &image));
    AT(panel->visual_count == 1);
    ANN(panel->background_visual);
    AT(panel->background_visual != gradient_visual);
    AT(panel->background_type == DVZ_PANEL_BACKGROUND_IMAGE);
    AT(panel->background_visual->type == DVZ_VISUAL_TYPE_IMAGE);
    AT(_visual_family_state(panel->background_visual)->field != NULL);
    AT(_visual_family_state(panel->background_visual)->field->desc.width == 2);
    AT(_visual_family_state(panel->background_visual)->field->desc.height == 2);
    AT(panel->visuals[0].visual == panel->background_visual);
    AT(panel->visuals[0].z_layer == -1);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);

    dvz_panel_clear_background(panel);
    AT(panel->visual_count == 0);
    AT(panel->background_visual == NULL);
    AT(panel->background_type == DVZ_PANEL_BACKGROUND_NONE);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_plot_clip_rect_metadata(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    AT(dvz_panel_set_padding(
        panel, &(DvzPanelReserve){
                   .left_px = 8.0f,
                   .right_px = 4.0f,
                   .top_px = 6.0f,
                   .bottom_px = 2.0f,
               }));
    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.25f, .right = 0.15f, .bottom = 0.10f,
                                        .top = 0.20f}));
    dvz_panel_set_background_color(panel, 0.1f, 0.2f, 0.3f, 1.0f);

    float pos[3] = {1.5f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float size = 8.0f;
    DvzVisual* point = dvz_point(scene, 0);
    AT(dvz_visual_set_data(point, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(point, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(point, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.plot_clip_rect", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    AT(dvz_frame_plan_node_count(plan) == 1);
    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(dvz_frame_plan_render_pass_role(render) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(render->u.render.visual_count == 2);
    AT(render->u.render.has_plot_desc);

    DvzPanelDesc plot_desc = _scene_panel_plot_desc(panel);
    DvzRect inner_rect = {0};
    AT(dvz_panel_inner_rect_px(panel, &inner_rect));
    AC(inner_rect.x, 8.0f, 1e-6);
    AC(inner_rect.y, 6.0f, 1e-6);
    AC(inner_rect.width, 116.0f, 1e-6);
    AC(inner_rect.height, 88.0f, 1e-6);
    DvzRect plot_rect = {0};
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    AC(plot_rect.x, 24.0f, 1e-6);
    AC(plot_rect.y, 15.6f, 1e-6);
    AC(plot_rect.width, 90.4f, 1e-6);
    AC(plot_rect.height, 73.6f, 1e-6);
    AC(plot_desc.x, 24.0f / 128.0f, 1e-6);
    AC(plot_desc.y, 15.6f / 96.0f, 1e-6);
    AC(plot_desc.width, 90.4f / 128.0f, 1e-6);
    AC(plot_desc.height, 73.6f / 96.0f, 1e-6);
    AC(render->u.render.desc.x, panel->desc.x, 1e-6);
    AC(render->u.render.desc.y, panel->desc.y, 1e-6);
    AC(render->u.render.desc.width, panel->desc.width, 1e-6);
    AC(render->u.render.desc.height, panel->desc.height, 1e-6);
    AC(render->u.render.plot_desc.x, plot_desc.x, 1e-6);
    AC(render->u.render.plot_desc.y, plot_desc.y, 1e-6);
    AC(render->u.render.plot_desc.width, plot_desc.width, 1e-6);
    AC(render->u.render.plot_desc.height, plot_desc.height, 1e-6);
    AT(render->u.render.visual_metadata[0].clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PANEL);
    AT(render->u.render.visual_metadata[1].clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PLOT);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 128;
    cfg.target_height = 96;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool saw_plot_scissor = false;
    uint32_t scissor_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type != DVZ_DRP2_COMMAND_SET_SCISSOR)
            continue;
        scissor_count++;
        if (fabsf(cmd->u.set_scissor.scissor[0] - plot_rect.x) < 1e-6f &&
            fabsf(cmd->u.set_scissor.scissor[1] - plot_rect.y) < 1e-6f &&
            fabsf(cmd->u.set_scissor.scissor[2] - plot_rect.width) < 1e-6f &&
            fabsf(cmd->u.set_scissor.scissor[3] - plot_rect.height) < 1e-6f)
            saw_plot_scissor = true;
    }
    AT(scissor_count >= 2);
    AT(saw_plot_scissor);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}

int test_scene_visual_local_transform_bounds_and_clear(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzVisual* visual = dvz_point(scene, 0);
    vec3 positions[2] = {{0.0f, -1.0f, 0.0f}, {1.0f, +1.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {4.0f, 4.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);

    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    glm_translate(transform, (vec3){2.0f, 0.0f, 0.0f});
    AT(dvz_visual_set_transform(visual, transform) == 0);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AC(bounds.min[0], 2.0, 1e-6);
    AC(bounds.max[0], 3.0, 1e-6);
    AC(bounds.min[1], -1.0, 1e-6);
    AC(bounds.max[1], +1.0, 1e-6);

    AT(dvz_visual_clear_transform(visual) == 0);
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AC(bounds.min[0], 0.0, 1e-6);
    AC(bounds.max[0], 1.0, 1e-6);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_visual_local_transform_emits_per_visual_mvp(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    vec3 pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor col[1] = {{255, 255, 255, 255}};
    float size[1] = {4.0f};
    DvzVisual* a = dvz_point(scene, 0);
    DvzVisual* b = dvz_point(scene, 0);
    AT(dvz_visual_set_data(a, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(a, "color", col, 1) == 0);
    AT(dvz_visual_set_data(a, "size", size, 1) == 0);
    AT(dvz_visual_set_data(b, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(b, "color", col, 1) == 0);
    AT(dvz_visual_set_data(b, "size", size, 1) == 0);

    mat4 ta = GLM_MAT4_IDENTITY_INIT;
    mat4 tb = GLM_MAT4_IDENTITY_INIT;
    glm_translate(ta, (vec3){1.0f, 0.0f, 0.0f});
    glm_translate(tb, (vec3){2.0f, 0.0f, 0.0f});
    AT(dvz_visual_set_transform(a, ta) == 0);
    AT(dvz_visual_set_transform(b, tb) == 0);
    AT(dvz_panel_add_visual(panel, a, NULL) == 0);
    AT(dvz_panel_add_visual(panel, b, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("visual.local.mvp", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(render->u.render.visual_count == 2);
    AT(render->u.render.visual_has_mvp[0]);
    AT(render->u.render.visual_has_mvp[1]);
    AC(render->u.render.visual_mvp[0].model[3][0], 1.0f, 1e-6);
    AC(render->u.render.visual_mvp[1].model[3][0], 2.0f, 1e-6);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_mesh_local_transform_without_instances(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    DvzVisual* mesh = dvz_mesh(scene, 0);

    vec3 positions[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    AT(dvz_visual_set_data(mesh, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(mesh, "color", colors, 3) == 0);
    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    glm_translate(transform, (vec3){0.0f, 3.0f, 0.0f});
    AT(dvz_visual_set_transform(mesh, transform) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("mesh.local.mvp", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(render->u.render.visual_count == 1);
    AT(render->u.render.visual_has_mvp[0]);
    AC(render->u.render.visual_mvp[0].model[3][1], 3.0f, 1e-6);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_controller_mode_fixed_emits_separate_mvp(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    /* One panel with a panzoom (APPLY) and a FIXED visual: the converter must allocate
     * two MVP UBOs, one per controller_mode. APPLY gets the panzoom MVP, FIXED gets
     * identity, and writes never overwrite each other. */
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    /* Note: we don't actually run a panzoom here — the controller_mode flag alone
     * determines whether the converter writes identity or the controller MVP. */

    float pos[3 * 3] = {0};
    DvzColor col[3] = {0};
    float sz[3] = {0};

    DvzVisual* v_apply = dvz_point(scene, 0);
    DvzVisual* v_fixed = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v_apply, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v_apply, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v_apply, "size", sz, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "size", sz, 3) == 0);

    AT(dvz_panel_add_visual(panel, v_apply, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v_fixed,
                            &(DvzVisualAttachDesc){.controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "controller_mode_test");
    ANN(json);

    /* Two distinct MVP UBOs (size 208 = sizeof(DvzMVP) + std140 padding) must be
     * created, one for APPLY and one for FIXED. The common set now also carries a
     * panel viewport uniform, so FIXED common bind groups are panel-scoped. */
    uint32_t mvp_buffers = 0;
    const char* p = json;
    while ((p = strstr(p, "\"size\": 208, \"usage\": [\"COPY_DST\"")) != NULL)
    {
        mvp_buffers++;
        p += 1;
    }
    AT(mvp_buffers == 2);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_one_pass_per_panel(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    float pos[3 * 3] = {0};
    DvzColor col[3]  = {0};
    float sz[3]      = {0};

    DvzVisual* v0 = dvz_point(scene, 0);
    DvzVisual* v1 = dvz_point(scene, 0);
    DvzVisual* v2 = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v0, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v0, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v0, "size",     sz,  3) == 0);
    AT(dvz_visual_set_data(v1, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v1, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v1, "size",     sz,  3) == 0);
    AT(dvz_visual_set_data(v2, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v2, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v2, "size",     sz,  3) == 0);
    AT(dvz_panel_add_visual(panel, v0, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v1, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v2, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    /* Exactly one BeginRenderPass and three Draws in that pass. */
    uint32_t pass_count = 0, draw_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            pass_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            draw_count++;
    }
    AT(pass_count == 1);
    AT(draw_count == 3);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0, 0, 0.5f, 1});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0, 0.5f, 1});

    float pos_l[3] = {-0.5f, 0.0f, 0.0f};
    float pos_r[3] = {0.5f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float sz = 6.0f;

    DvzVisual* vl = dvz_point(scene, 0);
    DvzVisual* vr = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vl, "position", pos_l, 1) == 0);
    AT(dvz_visual_set_data(vl, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vl, "size", &sz, 1) == 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vr, "size", &sz, 1) == 0);

    DvzVisualAttachDesc fixed = {.controller_mode = DVZ_CONTROLLER_FIXED};
    AT(dvz_panel_add_visual(left, vl, &fixed) == 0);
    AT(dvz_panel_add_visual(right, vr, &fixed) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t pass_count = 0, draw_count = 0, pipeline_count = 0, bind_group_count = 0;
    uint32_t viewport_count = 0, scissor_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            pass_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            draw_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
            pipeline_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            bind_group_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
            viewport_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
            scissor_count++;
    }
    AT(pass_count == 1);
    AT(draw_count == 2);
    AT(pipeline_count == 1);
    AT(bind_group_count == 2);
    AT(viewport_count == 2);
    AT(scissor_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_multi_panel_glsl_emits_viewport_scissor_commands(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 128, 64, 0);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});

    float pos_l[3] = {-0.6f, 0.0f, 0.0f};
    float pos_r[3] = {0.6f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float sz = 5.0f;

    DvzVisual* vl = dvz_point(scene, 0);
    DvzVisual* vr = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vl, "position", pos_l, 1) == 0);
    AT(dvz_visual_set_data(vl, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vl, "size", &sz, 1) == 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vr, "size", &sz, 1) == 0);
    AT(dvz_panel_add_visual(left, vl, NULL) == 0);
    AT(dvz_panel_add_visual(right, vr, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 128;
    cfg.target_height = 64;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t pass_count = 0, viewport_count = 0, scissor_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            pass_count++;
            AC(cmd->u.begin_render_pass.viewport[0], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[2], 1.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            AT(cmd->u.begin_render_pass.clear);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
        {
            if (viewport_count == 0)
            {
                AC(cmd->u.set_viewport.viewport[0], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 64.0f, 1e-5f);
                AC(cmd->u.set_viewport.viewport[3], 64.0f, 1e-5f);
            }
            else if (viewport_count == 1)
            {
                AC(cmd->u.set_viewport.viewport[0], 64.0f, 1e-5f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 64.0f, 1e-5f);
                AC(cmd->u.set_viewport.viewport[3], 64.0f, 1e-5f);
            }
            viewport_count++;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
        {
            if (scissor_count == 0)
            {
                AC(cmd->u.set_scissor.scissor[0], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[2], 64.0f, 1e-5f);
                AC(cmd->u.set_scissor.scissor[3], 64.0f, 1e-5f);
            }
            else if (scissor_count == 1)
            {
                AC(cmd->u.set_scissor.scissor[0], 64.0f, 1e-5f);
                AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[2], 64.0f, 1e-5f);
                AC(cmd->u.set_scissor.scissor[3], 64.0f, 1e-5f);
            }
            scissor_count++;
        }
    }

    AT(pass_count == 1);
    AT(viewport_count == 2);
    AT(scissor_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_overlapping_depth_panels_glsl_clear_depth(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 128, 128, 0);
    DvzPanel* main = dvz_panel_full(figure);
    DvzPanel* inset = dvz_panel(figure, (DvzPanelDesc){0.55f, 0.55f, 0.35f, 0.35f});
    AT(main != NULL);
    AT(inset != NULL);

    float pos_main[3] = {0.0f, 0.0f, 0.0f};
    float pos_inset[3] = {0.0f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float sz = 8.0f;

    DvzVisual* main_point = dvz_point(scene, 0);
    DvzVisual* inset_point = dvz_point(scene, 0);
    AT(dvz_visual_set_data(main_point, "position", pos_main, 1) == 0);
    AT(dvz_visual_set_data(main_point, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(main_point, "size", &sz, 1) == 0);
    AT(dvz_visual_set_data(inset_point, "position", pos_inset, 1) == 0);
    AT(dvz_visual_set_data(inset_point, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(inset_point, "size", &sz, 1) == 0);
    AT(dvz_panel_add_visual(main, main_point, NULL) == 0);
    AT(dvz_panel_add_visual(inset, inset_point, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t pass_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            continue;
        AT(cmd->u.begin_render_pass.has_depth_attachment);
        AT(cmd->u.begin_render_pass.depth_load_op == DVZ_DRP2_ATTACHMENT_LOAD_CLEAR);
        if (pass_count == 0)
        {
            AT(cmd->u.begin_render_pass.clear);
        }
        else if (pass_count == 1)
        {
            AT(!cmd->u.begin_render_pass.clear);
        }
        pass_count++;
    }
    AT(pass_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}
