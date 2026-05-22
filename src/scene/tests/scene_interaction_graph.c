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
        if (fabsf(cmd->u.set_scissor.scissor[0] - plot_desc.x) < 1e-6f &&
            fabsf(cmd->u.set_scissor.scissor[1] - plot_desc.y) < 1e-6f &&
            fabsf(cmd->u.set_scissor.scissor[2] - plot_desc.width) < 1e-6f &&
            fabsf(cmd->u.set_scissor.scissor[3] - plot_desc.height) < 1e-6f)
            saw_plot_scissor = true;
    }
    AT(scissor_count >= 2);
    AT(saw_plot_scissor);

    dvz_drp2_stream_destroy(stream);
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
                AC(cmd->u.set_viewport.viewport[2], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[3], 1.0f, 1e-6f);
            }
            else if (viewport_count == 1)
            {
                AC(cmd->u.set_viewport.viewport[0], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[3], 1.0f, 1e-6f);
            }
            viewport_count++;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
        {
            if (scissor_count == 0)
            {
                AC(cmd->u.set_scissor.scissor[0], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[2], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[3], 1.0f, 1e-6f);
            }
            else if (scissor_count == 1)
            {
                AC(cmd->u.set_scissor.scissor[0], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[2], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[3], 1.0f, 1e-6f);
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
