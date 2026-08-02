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
#include "annotation/prepare_internal.h"
#include "core/scene_notify_internal.h"
#include "domain/field_internal.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

typedef struct GridDestroyRequestProbe
{
    uint32_t calls;
    DvzFigure* last_figure;
} GridDestroyRequestProbe;


static void _grid_destroy_request_frame_callback(DvzFigure* figure, void* user_data)
{
    GridDestroyRequestProbe* probe = (GridDestroyRequestProbe*)user_data;
    ANN(probe);
    probe->calls++;
    probe->last_figure = figure;
}



static DvzVisual* _local_transform_audit_visual(DvzScene* scene, DvzVisualType type)
{
    ANN(scene);

    DvzVisual* visual = NULL;
    vec3 positions[4] = {
        {-0.50f, -0.50f, 0.0f},
        {+0.50f, -0.50f, 0.0f},
        {-0.50f, +0.50f, 0.0f},
        {+0.50f, +0.50f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 255, 255},
    };
    float sizes[4] = {6.0f, 6.0f, 6.0f, 6.0f};

    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        visual = dvz_point(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 1) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 1) != 0 ||
            dvz_visual_set_data(visual, "size", sizes, 1) != 0)
            return NULL;
        break;

    case DVZ_VISUAL_TYPE_PIXEL:
        visual = dvz_pixel(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 1) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 1) != 0 ||
            dvz_visual_set_data(visual, "pixel_size_px", sizes, 1) != 0)
            return NULL;
        break;

    case DVZ_VISUAL_TYPE_MARKER:
    {
        float angles[1] = {0.0f};
        uint32_t shapes[1] = {DVZ_MARKER_SHAPE_DISC};
        visual = dvz_marker(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 1) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 1) != 0 ||
            dvz_visual_set_data(visual, "size", sizes, 1) != 0 ||
            dvz_visual_set_data(visual, "angle", angles, 1) != 0 ||
            dvz_visual_set_data(visual, "shape", shapes, 1) != 0)
            return NULL;
        break;
    }

    case DVZ_VISUAL_TYPE_SEGMENT:
    {
        vec3 starts[1] = {{-0.50f, -0.25f, 0.0f}};
        vec3 ends[1] = {{+0.50f, +0.25f, 0.0f}};
        visual = dvz_segment(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position_start", starts, 1) != 0 ||
            dvz_visual_set_data(visual, "position_end", ends, 1) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 1) != 0 ||
            dvz_visual_set_data(visual, "stroke_width_px", sizes, 1) != 0)
            return NULL;
        break;
    }

    case DVZ_VISUAL_TYPE_PATH:
        visual = dvz_path(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 3) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 3) != 0 ||
            dvz_visual_set_data(visual, "stroke_width_px", sizes, 3) != 0)
            return NULL;
        break;

    case DVZ_VISUAL_TYPE_IMAGE:
    {
        vec2 texcoords[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};
        uint8_t pixels[4 * 4 * 4];
        dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));
        visual = dvz_image(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 4) != 0 ||
            dvz_visual_set_data(visual, "texcoords", texcoords, 4) != 0 ||
            _scene_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 4, 4, 4u * 4u * 4u) != 0)
            return NULL;
        break;
    }

    case DVZ_VISUAL_TYPE_MESH:
        visual = dvz_mesh(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 3) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 3) != 0)
            return NULL;
        break;

    case DVZ_VISUAL_TYPE_PRIMITIVE:
        visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 3) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 3) != 0)
            return NULL;
        break;

    case DVZ_VISUAL_TYPE_SPHERE:
        visual = dvz_sphere(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 1) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 1) != 0 ||
            dvz_visual_set_data(visual, "radius", sizes, 1) != 0)
            return NULL;
        break;

    case DVZ_VISUAL_TYPE_SPLAT:
    {
        vec2 sigma[1] = {{5.0f, 4.0f}};
        float angles[1] = {0.0f};
        visual = dvz_splat(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 1) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 1) != 0 ||
            dvz_visual_set_data(visual, "sigma", sigma, 1) != 0 ||
            dvz_visual_set_data(visual, "angle", angles, 1) != 0)
            return NULL;
        break;
    }

    case DVZ_VISUAL_TYPE_VECTOR:
    {
        vec3 vectors[1] = {{0.75f, 0.25f, 0.0f}};
        visual = dvz_vector(scene, 0);
        if (visual == NULL || dvz_visual_set_data(visual, "position", positions, 1) != 0 ||
            dvz_visual_set_data(visual, "vector", vectors, 1) != 0 ||
            dvz_visual_set_data(visual, "color", colors, 1) != 0 ||
            dvz_visual_set_data(visual, "stroke_width_px", sizes, 1) != 0)
            return NULL;
        break;
    }

    default:
        return NULL;
    }

    return visual;
}


static const DvzFramePlanNode* _first_render_with_visual(const DvzFramePlan* plan)
{
    ANN(plan);
    for (uint32_t i = 0; i < dvz_frame_plan_node_count(plan); i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        if (node != NULL && node->type == DVZ_FRAME_PLAN_NODE_RENDER &&
            node->u.render.visual_count > 0)
            return node;
    }
    return NULL;
}


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


int test_scene_lifetime_local_ids(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    AT(dvz_scene_id(NULL) == DVZ_ID_NONE);
    AT(dvz_figure_id(NULL) == DVZ_ID_NONE);
    AT(dvz_panel_id(NULL) == DVZ_ID_NONE);
    AT(dvz_visual_id(NULL) == DVZ_ID_NONE);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzId scene_id = dvz_scene_id(scene);
    AT(scene_id != DVZ_ID_NONE);

    DvzFigure* figure = dvz_figure(scene, 64, 32, 0);
    ANN(figure);
    DvzId figure_id = dvz_figure_id(figure);
    AT(figure_id != DVZ_ID_NONE);
    AT(figure_id != scene_id);

    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    DvzId panel_id = dvz_panel_id(panel);
    AT(panel_id != DVZ_ID_NONE);
    AT(panel_id != scene_id);
    AT(panel_id != figure_id);

    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.width = 2;
    field_desc.height = 2;
    DvzSampledField* field = dvz_sampled_field(scene, &field_desc);
    ANN(field);
    DvzId field_id = dvz_sampled_field_id(field);
    AT(field_id != DVZ_ID_NONE);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                 .kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    DvzId scale_id = dvz_scale_id(scale);
    AT(scale_id != DVZ_ID_NONE);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzId colormap_id = dvz_colormap_id(colormap);
    AT(colormap_id != DVZ_ID_NONE);

    DvzColorbar* colorbar = dvz_colorbar(panel, scale, NULL);
    ANN(colorbar);
    DvzId colorbar_id = dvz_colorbar_id(colorbar);
    AT(colorbar_id != DVZ_ID_NONE);

    DvzScale* categorical = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                 .kind = DVZ_SCALE_CATEGORICAL});
    ANN(categorical);
    DvzLegend* legend = dvz_legend(panel, categorical, NULL);
    ANN(legend);
    DvzId legend_id = dvz_legend_id(legend);
    AT(legend_id != DVZ_ID_NONE);

    DvzText* text = dvz_text(panel, 0);
    ANN(text);
    DvzId text_id = dvz_text_id(text);
    AT(text_id != DVZ_ID_NONE);

    DvzAnnotationDesc annotation_desc = dvz_annotation_desc();
    annotation_desc.text = "id";
    DvzAnnotation* annotation = dvz_annotation(panel, &annotation_desc);
    ANN(annotation);
    DvzId annotation_id = dvz_annotation_id(annotation);
    AT(annotation_id != DVZ_ID_NONE);

    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    DvzId controller_id = dvz_controller_id(controller);
    AT(controller_id != DVZ_ID_NONE);

    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    DvzId visual_id = dvz_visual_id(visual);
    AT(visual_id != DVZ_ID_NONE);
    AT(visual_id != scene_id);
    AT(visual_id != figure_id);
    AT(visual_id != panel_id);
    AT(visual_id != field_id);
    AT(visual_id != scale_id);
    AT(visual_id != colormap_id);
    AT(visual_id != colorbar_id);
    AT(visual_id != legend_id);
    AT(visual_id != text_id);
    AT(visual_id != annotation_id);
    AT(visual_id != controller_id);
    AT(_scene_visual_public_id(scene, visual) == visual_id);

    dvz_sampled_field_destroy(field);
    AT(dvz_sampled_field_id(field) == DVZ_ID_NONE);
    dvz_colorbar_destroy(colorbar);
    AT(dvz_colorbar_id(colorbar) == DVZ_ID_NONE);
    dvz_legend_destroy(legend);
    AT(dvz_legend_id(legend) == DVZ_ID_NONE);
    dvz_text_destroy(text);
    AT(dvz_text_id(text) == DVZ_ID_NONE);
    dvz_annotation_destroy(annotation);
    AT(dvz_annotation_id(annotation) == DVZ_ID_NONE);

    dvz_figure_destroy(figure);
    AT(dvz_figure_id(figure) == DVZ_ID_NONE);
    AT(dvz_panel_id(panel) == DVZ_ID_NONE);
    DvzFigure* reused = dvz_figure(scene, 128, 64, 0);
    ANN(reused);
    AT(reused == figure);
    AT(dvz_figure_id(reused) != figure_id);

    dvz_visual_destroy(visual);
    AT(dvz_visual_id(visual) == DVZ_ID_NONE);
    DvzVisual* next_visual = dvz_point(scene, 0);
    ANN(next_visual);
    AT(dvz_visual_id(next_visual) != DVZ_ID_NONE);
    AT(dvz_visual_id(next_visual) != visual_id);

    dvz_scale_destroy(scale);
    AT(dvz_scale_id(scale) == DVZ_ID_NONE);
    dvz_colormap_destroy(colormap);
    AT(dvz_colormap_id(colormap) == DVZ_ID_NONE);

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
                  .bottom_px = 30.0f}) == DVZ_OK);
    AT(dvz_grid_set_gutter(grid, 10.0f, 20.0f) == DVZ_OK);
    AT(dvz_grid_set_col_size(grid, 0, DVZ_GRID_SIZE_WEIGHT, 1.0f) == DVZ_OK);
    AT(dvz_grid_set_col_size(grid, 1, DVZ_GRID_SIZE_WEIGHT, 2.0f) == DVZ_OK);
    AT(dvz_grid_set_col_size(grid, 2, DVZ_GRID_SIZE_FIXED_PX, 60.0f) == DVZ_OK);

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
    AT(dvz_grid_set_col_size(grid, 0, DVZ_GRID_SIZE_WEIGHT, 0.0f) == DVZ_ERROR);
    AT(dvz_grid_set_row_size(grid, 0, DVZ_GRID_SIZE_FIXED_PX, -1.0f) == DVZ_ERROR);
    AT(dvz_grid_set_gutter(grid, -1.0f, 0.0f) == DVZ_ERROR);
    AT(dvz_grid_set_margins(grid, &(DvzPanelReserve){.left_px = -1.0f}) == DVZ_ERROR);

    DvzPanelDesc desc = {0};
    AT(!dvz_grid_resolve(
        grid, 64, 64, (DvzGridCell){.row = 1, .col = 1, .row_span = 2, .col_span = 1},
        &desc));
    AT(!dvz_grid_resolve(
        grid, 0, 64, (DvzGridCell){.row = 0, .col = 0, .row_span = 1, .col_span = 1},
        &desc));

    AT(dvz_grid_set_margins(
        grid, &(DvzPanelReserve){.left_px = 40.0f, .right_px = 40.0f}) == DVZ_OK);
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

    AT(dvz_grid_set_col_size(grid, 0, DVZ_GRID_SIZE_FIXED_PX, 60.0f) == DVZ_OK);

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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
    AT(dvz_grid_set_gutter(grid, 10.0f, 0.0f) == DVZ_OK);
    AT(dvz_grid_set_col_size(grid, 1, DVZ_GRID_SIZE_FIXED_PX, 60.0f) == DVZ_OK);

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

    AT(dvz_panel_set_desc(
           data, &(DvzPanelDesc){.x = 0.1f, .y = 0.1f, .width = 0.8f, .height = 0.8f}) ==
       DVZ_OK);
    dvz_figure_resize(figure, 500, 100);
    AC(data->desc.x, 0.1f, 1e-6f);
    AC(data->desc.width, 0.8f, 1e-6f);
    AC(colorbar->desc.x, 0.88f, 1e-6f);
    AC(colorbar->desc.width, 0.12f, 1e-6f);

    AT(dvz_panel_set_desc(
           colorbar, &(DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 0.0f, .height = 1.0f}) !=
       DVZ_OK);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_grid_destroy_detaches_panels_and_reuses_slot(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    dvz_grid_destroy(NULL);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzGrid* grid = dvz_figure_grid(figure, 1, 2);
    ANN(grid);
    DvzPanel* left = dvz_grid_panel(grid, 0, 0);
    DvzPanel* right = dvz_grid_panel(grid, 0, 1);
    ANN(left);
    ANN(right);
    AT(left->grid == grid);
    AT(right->grid == grid);
    AT(grid->panel_count == 2);

    GridDestroyRequestProbe probe = {0};
    AT(_scene_add_request_frame_callback(scene, _grid_destroy_request_frame_callback, &probe));

    dvz_grid_destroy(grid);
    AT(probe.calls == 1);
    AT(probe.last_figure == figure);
    AT(left->grid == NULL);
    AT(right->grid == NULL);
    AT(grid->figure == NULL);

    dvz_grid_destroy(grid);
    AT(probe.calls == 1);

    DvzGrid* reused = dvz_figure_grid(figure, 2, 1);
    AT(reused == grid);
    AT(reused->figure == figure);
    AT(reused->rows == 2);
    AT(reused->cols == 1);
    AT(reused->panel_count == 0);

    _scene_remove_request_frame_callback(scene, _grid_destroy_request_frame_callback, &probe);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_grid_destroy_detached_panel_still_emits(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzGrid* grid = dvz_figure_grid(figure, 1, 1);
    ANN(grid);
    DvzPanel* panel = dvz_grid_panel(grid, 0, 0);
    ANN(panel);

    vec3 pos = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 255, 255};
    float size = 5.0f;
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    AT(dvz_visual_set_data(visual, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    dvz_grid_destroy(grid);
    AT(panel->grid == NULL);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    _test_scene_stream_destroy(stream);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_destroy_removes_grid_attachment(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzGrid* grid = dvz_figure_grid(figure, 1, 2);
    ANN(grid);
    DvzPanel* left = dvz_grid_panel(grid, 0, 0);
    DvzPanel* right = dvz_grid_panel(grid, 0, 1);
    ANN(left);
    ANN(right);
    AT(grid->panel_count == 2);

    dvz_panel_destroy(left);
    AT(left->grid == NULL);
    AT(grid->panel_count == 1);
    AT(grid->panels[0].panel == right);

    dvz_figure_resize(figure, 400, 100);
    AT(right->figure == figure);
    AT(right->grid == grid);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_reference_grid_api_and_geometry(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzReferenceGridDesc invalid = dvz_reference_grid_desc();
    invalid.plane = DVZ_REFERENCE_GRID_CUSTOM;
    invalid.axis_u[0] = 0.0f;
    invalid.axis_u[1] = 0.0f;
    invalid.axis_u[2] = 0.0f;
    AT(dvz_reference_grid(panel, &invalid) == NULL);

    DvzReferenceGridDesc desc = dvz_reference_grid_desc();
    desc.plane = DVZ_REFERENCE_GRID_XZ;
    desc.origin[1] = -0.50f;
    desc.size[0] = 5.0f;
    desc.size[1] = 1.0f;
    desc.spacing = 0.50f;
    desc.major_every = 2;
    desc.depth_test = false;

    DvzReferenceGrid* grid = dvz_reference_grid(panel, &desc);
    ANN(grid);
    ANN(grid->visual);
    AT(scene->reference_grid_count == 1);
    AT(grid->line_count == 14);
    AT(dvz_visual_alpha_mode(grid->visual) == DVZ_ALPHA_BLENDED);
    AT(!dvz_visual_depth_test(grid->visual));
    AT(panel->visual_count == 1);
    AT(panel->visuals[0].visual == grid->visual);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_APPLY_VIEW_PROJ);

    DvzVisualDataView start_view = {0};
    DvzVisualDataView end_view = {0};
    DvzVisualDataView color_view = {0};
    DvzVisualDataView width_view = {0};
    AT(dvz_visual_data(grid->visual, "position_start", &start_view) == 0);
    AT(dvz_visual_data(grid->visual, "position_end", &end_view) == 0);
    AT(dvz_visual_data(grid->visual, "color", &color_view) == 0);
    AT(dvz_visual_data(grid->visual, "stroke_width_px", &width_view) == 0);
    AT(start_view.item_count == 14);
    AT(end_view.item_count == 14);
    AT(color_view.item_count == 14);
    AT(width_view.item_count == 14);

    const float* starts = (const float*)start_view.data;
    const float* ends = (const float*)end_view.data;
    const float* widths = (const float*)width_view.data;
    ANN(starts);
    ANN(ends);
    ANN(widths);
    AC(starts[0], -2.5f, 1e-6f);
    AC(starts[1], -0.5f, 1e-6f);
    AC(starts[2], -0.5f, 1e-6f);
    AC(ends[0], -2.5f, 1e-6f);
    AC(ends[1], -0.5f, 1e-6f);
    AC(ends[2], +0.5f, 1e-6f);
    AC(widths[0], desc.minor_width_px, 1e-6f);
    AC(widths[1], desc.major_width_px, 1e-6f);
    AC(widths[2], desc.minor_width_px, 1e-6f);
    AC(widths[3], desc.major_width_px, 1e-6f);
    AC(widths[5], desc.axis_width_px, 1e-6f);
    AC(widths[7], desc.major_width_px, 1e-6f);
    AC(widths[8], desc.minor_width_px, 1e-6f);
    AC(widths[9], desc.major_width_px, 1e-6f);
    AC(widths[10], desc.minor_width_px, 1e-6f);
    AC(widths[11], desc.minor_width_px, 1e-6f);
    AC(widths[12], desc.axis_width_px, 1e-6f);
    AC(widths[13], desc.minor_width_px, 1e-6f);

    AT(dvz_reference_grid_set_visible(grid, false) == DVZ_OK);
    AT(!grid->visible);
    AT(!grid->visual->visible);
    AT(dvz_reference_grid_set_visible(grid, true) == DVZ_OK);
    AT(grid->visible);
    AT(grid->visual->visible);

    dvz_reference_grid_destroy(grid);
    AT(!scene->reference_grids[0].active);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_controller_mode_view_proj_strips_panel_model(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 128, 128, 0);
    DvzPanel* panel = dvz_panel_full(figure);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[2] = 3.0f;
    camera_desc.view.target[0] = 0.0f;
    camera_desc.view.target[1] = 0.0f;
    camera_desc.view.target[2] = 0.0f;
    camera_desc.projection.near_clip = 0.1f;
    camera_desc.projection.far_clip = 100.0f;
    AT(dvz_panel_set_camera_desc(panel, &camera_desc) == 0);

    DvzController* controller = dvz_arcball(scene, NULL);
    ANN(controller);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    ANN(arcball);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);
    dvz_arcball_set(arcball, (vec3){0.4f, -0.3f, 0.2f});
    dvz_arcball_zoom(arcball, 1.8f);
    dvz_arcball_pan(arcball, (vec2){0.25f, -0.15f});

    DvzMVP camera_mvp = {0};
    glm_mat4_identity(camera_mvp.model);
    glm_mat4_identity(camera_mvp.view);
    glm_mat4_identity(camera_mvp.proj);
    dvz_camera_mvp(panel->camera, &camera_mvp);

    vec3 pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor col[1] = {{255, 255, 255, 255}};
    float size[1] = {4.0f};
    DvzVisual* visual = dvz_point(scene, 0);
    AT(dvz_visual_set_data(visual, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", col, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", size, 1) == 0);

    mat4 local = GLM_MAT4_IDENTITY_INIT;
    glm_translate(local, (vec3){2.0f, 0.0f, 0.0f});
    AT(dvz_visual_set_transform(visual, local) == 0);

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.controller_mode = DVZ_CONTROLLER_APPLY_VIEW_PROJ;
    attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    AT(dvz_panel_add_visual(panel, visual, &attach) == 0);

    DvzFramePlan* plan = dvz_frame_plan("controller.view_proj", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    const DvzFramePlanNode* render = _first_render_with_visual(plan);
    ANN(render);
    AT(render->u.render.visual_count == 1);
    AT(render->u.render.controller_modes[0] == DVZ_CONTROLLER_APPLY_VIEW_PROJ);
    AT(render->u.render.visual_has_mvp[0]);

    AT(fabsf(render->u.render.apply_mvp.model[0][0] - 1.0f) > 1e-3f);
    AT(fabsf(render->u.render.apply_mvp.view[3][0] - camera_mvp.view[3][0]) > 1e-3f);
    AT(fabsf(render->u.render.apply_mvp.view[3][2] - camera_mvp.view[3][2]) > 1e-3f);
    AC(render->u.render.visual_mvp[0].model[0][0], 1.0f, 1e-6f);
    AC(render->u.render.visual_mvp[0].model[1][1], 1.0f, 1e-6f);
    AC(render->u.render.visual_mvp[0].model[2][2], 1.0f, 1e-6f);
    AC(render->u.render.visual_mvp[0].model[3][0], 2.0f, 1e-6f);
    AC(
        render->u.render.visual_mvp[0].view[3][2], render->u.render.apply_mvp.view[3][2],
        1e-6f);
    AC(
        render->u.render.visual_mvp[0].view[3][0], render->u.render.apply_mvp.view[3][0],
        1e-6f);
    AC(
        render->u.render.visual_mvp[0].proj[0][0], render->u.render.apply_mvp.proj[0][0],
        1e-6f);
    AC(render->u.render.apply_mvp.proj[0][0], camera_mvp.proj[0][0], 1e-6f);
    AC(render->u.render.apply_mvp.proj[2][0], camera_mvp.proj[2][0], 1e-6f);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_figure_destroy_cascades_and_reuses_slot(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    dvz_figure_destroy(NULL);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzGrid* grid = dvz_figure_grid(figure, 1, 1);
    ANN(grid);
    DvzPanel* panel = dvz_grid_panel(grid, 0, 0);
    ANN(panel);

    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) == 0);

    static const char* shader =
        "#version 450\n"
        "layout(local_size_x = 1) in;\n"
        "void main() {}\n";
    DvzSceneCompute* compute = dvz_scene_compute(
        scene, &(DvzSceneComputeDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneComputeDesc),
                   .label = "figure_destroy_compute",
                   .shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL,
                   .shader_source = shader,
                   .dispatch = {1, 1, 1},
               });
    ANN(compute);
    AT(dvz_figure_add_compute(figure, compute) == DVZ_OK);
    AT(figure->compute_count == 1);

    GridDestroyRequestProbe probe = {0};
    AT(_scene_add_request_frame_callback(scene, _grid_destroy_request_frame_callback, &probe));

    dvz_figure_destroy(figure);
    AT(probe.calls == 0);
    AT(dvz_figure_scene(figure) == NULL);
    AT(panel->figure == NULL);
    AT(grid->figure == NULL);
    AT(dvz_controller_type(controller) == DVZ_CONTROLLER_TYPE_PANZOOM);
    AT(compute->scene == scene);

    dvz_figure_destroy(figure);
    AT(probe.calls == 0);

    DvzFigure* reused = dvz_figure(scene, 300, 150, 0);
    AT(reused == figure);
    AT(reused->panel_count == 0);
    AT(reused->grid_count == 0);
    AT(reused->compute_count == 0);
    uint32_t width = 0;
    uint32_t height = 0;
    dvz_figure_size(reused, &width, &height);
    AT(width == 300);
    AT(height == 150);

    DvzPanel* reused_panel = dvz_panel_full(reused);
    ANN(reused_panel);
    AT(dvz_panel_add_visual(reused_panel, visual, NULL) == 0);
    AT(dvz_panel_bind_controller(reused_panel, controller, DVZ_DIM_MASK_XY) == 0);
    AT(dvz_figure_add_compute(reused, compute) == DVZ_OK);

    _scene_remove_request_frame_callback(scene, _grid_destroy_request_frame_callback, &probe);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

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

    AT(dvz_panel_add_visual(panel, v_front,  &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = +1}) == 0);
    AT(dvz_panel_add_visual(panel, v_behind, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = -1}) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_background_color_creates_fixed_quad(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

    /* Initially no visuals. */
    AT(panel->visual_count == 0);
    AT(panel->background_visual == NULL);
    DvzPanelBackgroundDesc background_out = {0};
    AT(dvz_panel_background(panel, &background_out));
    AT(background_out.type == DVZ_PANEL_BACKGROUND_NONE);
    AT(!dvz_panel_background(NULL, &background_out));
    AT(!dvz_panel_background(panel, NULL));

    /* First call: creates a hidden background visual at z_layer=-1, FIXED. */
    DvzColor bg_color = dvz_color_from_unit(0.1f, 0.2f, 0.3f, 1.0f);
    AT(dvz_panel_set_background_color(panel, bg_color) == DVZ_OK);
    AT(panel->visual_count == 1);
    ANN(panel->background_visual);
    AT(panel->visuals[0].visual == panel->background_visual);
    AT(panel->visuals[0].z_layer == -1);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);
    AT(panel->visuals[0].has_generated_role);
    AT(panel->visuals[0].generated_role == DVZ_GENERATED_VISUAL_PANEL_BACKGROUND);
    AT(panel->background_type == DVZ_PANEL_BACKGROUND_COLOR);
    AT(dvz_panel_background(panel, &background_out));
    AT(background_out.type == DVZ_PANEL_BACKGROUND_COLOR);
    AC(background_out.color[0], (float)bg_color.r / 255.0f, 1e-6f);
    AC(background_out.color[1], (float)bg_color.g / 255.0f, 1e-6f);
    AC(background_out.color[2], (float)bg_color.b / 255.0f, 1e-6f);

    /* Second call with a different color: updates in place, no new visual. */
    DvzVisual* before = panel->background_visual;
    AT(dvz_panel_set_background_color(panel, dvz_color_from_unit(0.9f, 0.8f, 0.7f, 1.0f))
       == DVZ_OK);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

    DvzPanelBackgroundDesc gradient = {DVZ_STRUCT_INIT_FIELDS(DvzPanelBackgroundDesc),
        .type = DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT,
        .gradient = {
            .start = {0.0f, 0.0f},
            .end = {1.0f, 0.0f},
            .color0 = {1.0f, 0.0f, 0.0f, 1.0f},
            .color1 = {0.0f, 0.0f, 1.0f, 1.0f},
        },
    };
    AT(dvz_panel_set_background(panel, &gradient) == DVZ_OK);
    AT(panel->visual_count == 1);
    ANN(panel->background_visual);
    AT(panel->background_type == DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT);
    DvzPanelBackgroundDesc background_out = {0};
    AT(dvz_panel_background(panel, &background_out));
    AT(background_out.type == DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT);
    AC(background_out.gradient.end[0], 1.0f, 1e-6f);
    AC(background_out.gradient.color1[2], 1.0f, 1e-6f);
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
    AT(dvz_panel_set_background(panel, &gradient) == DVZ_OK);
    AT(panel->visual_count == 1);
    AT(panel->background_visual == gradient_visual);
    AT(dvz_panel_background(panel, &background_out));
    AC(background_out.gradient.end[0], 0.0f, 1e-6f);
    AC(background_out.gradient.end[1], 1.0f, 1e-6f);

    uint8_t pixels[2 * 2 * 4] = {
        255, 0, 0, 255, 0, 255, 0, 255,
        0, 0, 255, 255, 255, 255, 255, 255,
    };
    DvzPanelBackgroundDesc image = {DVZ_STRUCT_INIT_FIELDS(DvzPanelBackgroundDesc),
        .type = DVZ_PANEL_BACKGROUND_IMAGE,
        .image = {.rgba = pixels, .width = 2, .height = 2},
    };
    AT(dvz_panel_set_background(panel, &image) == DVZ_OK);
    AT(panel->visual_count == 1);
    ANN(panel->background_visual);
    AT(panel->background_visual != gradient_visual);
    AT(panel->background_type == DVZ_PANEL_BACKGROUND_IMAGE);
    AT(dvz_panel_background(panel, &background_out));
    AT(background_out.type == DVZ_PANEL_BACKGROUND_IMAGE);
    AT(background_out.image.rgba == NULL);
    AT(background_out.image.width == 2);
    AT(background_out.image.height == 2);
    AT(panel->background_visual->type == DVZ_VISUAL_TYPE_IMAGE);
    AT(_visual_family_state(panel->background_visual)->field != NULL);
    AT(_visual_family_state(panel->background_visual)->field->desc.width == 2);
    AT(_visual_family_state(panel->background_visual)->field->desc.height == 2);
    AT(panel->visuals[0].visual == panel->background_visual);
    AT(panel->visuals[0].z_layer == -1);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);

    AT(dvz_panel_clear_background(panel) == DVZ_OK);
    AT(panel->visual_count == 0);
    AT(panel->background_visual == NULL);
    AT(panel->background_type == DVZ_PANEL_BACKGROUND_NONE);
    AT(dvz_panel_background(panel, &background_out));
    AT(background_out.type == DVZ_PANEL_BACKGROUND_NONE);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_border_creates_fixed_overlay(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 100, 80, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

    DvzPanelBorderDesc border = dvz_panel_border_desc();
    border.color = dvz_color_rgba(10, 20, 30, 255);
    border.width_px = 2.0f;
    border.inset_px = 4.0f;

    AT(panel->border_visual == NULL);
    DvzPanelBorderDesc border_out = {0};
    AT(dvz_panel_border(panel, &border_out));
    AT(!border_out.visible);
    AT(!dvz_panel_border(NULL, &border_out));
    AT(!dvz_panel_border(panel, NULL));
    AT(dvz_panel_set_border(panel, &border) == DVZ_OK);
    AT(panel->visual_count == 1);
    ANN(panel->border_visual);
    AT(panel->visuals[0].visual == panel->border_visual);
    AT(panel->visuals[0].z_layer > 0);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);
    AT(panel->visuals[0].has_generated_role);
    AT(panel->visuals[0].generated_role == DVZ_GENERATED_VISUAL_PANEL_BORDER);
    AT(panel->border_visual->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(panel->border.visible);
    AT(panel->border.width_px == 2.0f);
    AT(dvz_panel_border(panel, &border_out));
    AT(border_out.visible);
    AC(border_out.width_px, 2.0f, 1e-6f);
    AC(border_out.inset_px, 4.0f, 1e-6f);
    AT(border_out.color.r == 10);

    int start_idx = _attr_index(panel->border_visual, "position_start");
    int width_idx = _attr_index(panel->border_visual, "line_width");
    int color_idx = _attr_index(panel->border_visual, "color");
    AT(start_idx >= 0);
    AT(width_idx >= 0);
    AT(color_idx >= 0);
    const float* starts = (const float*)panel->border_visual->attrs[start_idx].data;
    const float* widths = (const float*)panel->border_visual->attrs[width_idx].data;
    const DvzColor* colors = (const DvzColor*)panel->border_visual->attrs[color_idx].data;
    ANN(starts);
    ANN(widths);
    ANN(colors);
    AC(starts[0], -0.92f, 1e-6f);
    AC(widths[0], 2.0f, 1e-6f);
    AT(colors[0].r == 10);
    AT(colors[0].g == 20);
    AT(colors[0].b == 30);

    DvzVisual* before = panel->border_visual;
    border.inset_px = 8.0f;
    AT(dvz_panel_set_border(panel, &border) == DVZ_OK);
    AT(panel->visual_count == 1);
    AT(panel->border_visual == before);
    AT(dvz_panel_border(panel, &border_out));
    AC(border_out.inset_px, 8.0f, 1e-6f);
    starts = (const float*)panel->border_visual->attrs[start_idx].data;
    AC(starts[0], -0.84f, 1e-6f);

    AT(dvz_figure_resize(figure, 200, 80) == DVZ_OK);
    starts = (const float*)panel->border_visual->attrs[start_idx].data;
    AC(starts[0], -0.92f, 1e-6f);

    AT(dvz_panel_clear_border(panel) == DVZ_OK);
    AT(panel->visual_count == 0);
    AT(panel->border_visual == NULL);
    AT(!panel->border.visible);
    AT(dvz_panel_border(panel, &border_out));
    AT(!border_out.visible);

    border.visible = false;
    AT(dvz_panel_set_border(panel, &border) == DVZ_OK);
    AT(panel->visual_count == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_plot_clip_rect_metadata(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
    AT(dvz_panel_set_padding(
        panel, &(DvzPanelReserve){
                   .left_px = 8.0f,
                   .right_px = 4.0f,
                   .top_px = 6.0f,
                   .bottom_px = 2.0f,
               }) == DVZ_OK);
    AT(dvz_panel_set_reserve(
        panel, &(DvzPanelReserve){.left_px = 16.0f, .right_px = 9.6f, .bottom_px = 4.8f,
                                  .top_px = 9.6f}) == DVZ_OK);
    AT(dvz_panel_set_background_color(panel, dvz_color_from_unit(0.1f, 0.2f, 0.3f, 1.0f))
       == DVZ_OK);

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
    AT(_frame_plan_render_pass_role(render) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
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
    AT(render->u.render.visual_metadata[0].viewport_rect == DVZ_FRAME_PLAN_VIEWPORT_PANEL);
    AT(render->u.render.visual_metadata[1].clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PLOT);
    AT(render->u.render.visual_metadata[1].viewport_rect == DVZ_FRAME_PLAN_VIEWPORT_PLOT);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool saw_plot_viewport = false;
    bool saw_plot_scissor = false;
    bool saw_panel_viewport_uniform = false;
    bool saw_plot_viewport_uniform = false;
    uint32_t viewport_count = 0;
    uint32_t scissor_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
        {
            viewport_count++;
            if (fabsf(cmd->u.set_viewport.viewport[0] - plot_rect.x) < 1e-6f &&
                fabsf(cmd->u.set_viewport.viewport[1] - plot_rect.y) < 1e-6f &&
                fabsf(cmd->u.set_viewport.viewport[2] - plot_rect.width) < 1e-6f &&
                fabsf(cmd->u.set_viewport.viewport[3] - plot_rect.height) < 1e-6f)
                saw_plot_viewport = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
        {
            scissor_count++;
            if (fabsf(cmd->u.set_scissor.scissor[0] - plot_rect.x) < 1e-6f &&
                fabsf(cmd->u.set_scissor.scissor[1] - plot_rect.y) < 1e-6f &&
                fabsf(cmd->u.set_scissor.scissor[2] - plot_rect.width) < 1e-6f &&
                fabsf(cmd->u.set_scissor.scissor[3] - plot_rect.height) < 1e-6f)
                saw_plot_scissor = true;
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size == sizeof(DvzSceneViewportUniform))
        {
            const DvzSceneViewportUniform* viewport =
                (const DvzSceneViewportUniform*)cmd->u.write_buffer.data_raw;
            ANN(viewport);
            if (fabsf(viewport->x) < 1e-6f && fabsf(viewport->y) < 1e-6f &&
                fabsf(viewport->width - 128.0f) < 1e-6f &&
                fabsf(viewport->height - 96.0f) < 1e-6f)
            {
                saw_panel_viewport_uniform = true;
            }
            if (fabsf(viewport->x - plot_rect.x) < 1e-6f &&
                fabsf(viewport->y - plot_rect.y) < 1e-6f &&
                fabsf(viewport->width - plot_rect.width) < 1e-6f &&
                fabsf(viewport->height - plot_rect.height) < 1e-6f)
            {
                saw_plot_viewport_uniform = true;
            }
        }
    }
    AT(viewport_count >= 2);
    AT(saw_plot_viewport);
    AT(scissor_count >= 2);
    AT(saw_plot_scissor);
    AT(saw_panel_viewport_uniform);
    AT(saw_plot_viewport_uniform);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_frame_snapshot_core(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 400, 300, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.25f, 0.20f, 0.50f, 0.60f});
    ANN(panel);

    AT(dvz_panel_set_padding(
        panel, &(DvzPanelReserve){
                   .left_px = 10.0f,
                   .right_px = 6.0f,
                   .top_px = 8.0f,
                   .bottom_px = 4.0f,
               }) == DVZ_OK);
    AT(dvz_panel_set_reserve(
        panel, &(DvzPanelReserve){
                   .left_px = 20.0f,
                   .right_px = 12.0f,
                   .top_px = 16.0f,
                   .bottom_px = 10.0f,
               }) == DVZ_OK);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, -5.0, 15.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, 10.0, -10.0) == 0);
    DvzPanelView2DDesc view = dvz_panel_view2d_desc();
    AT(dvz_panel_set_view2d(panel, &view) == 0);
    DvzPanelView2DState view_state = {0};
    AT(dvz_panel_view2d_state(panel, &view_state));
    AT(view_state.struct_size == DVZ_STRUCT_SIZE(DvzPanelView2DState));
    AT(view_state.view_id != DVZ_ID_NONE);
    AT(view_state.revision > 0);
    AT(view_state.enabled);
    AT(view_state.has_domain_x);
    AT(view_state.has_domain_y);
    AC(view_state.domain_x[0], -5.0, 1e-9);
    AC(view_state.domain_x[1], 15.0, 1e-9);
    AC(view_state.domain_y[0], 10.0, 1e-9);
    AC(view_state.domain_y[1], -10.0, 1e-9);

    DvzPanelFrameSnapshot* snapshot = dvz_panel_resolve_frame(panel);
    ANN(snapshot);
    DvzPanelFrameInfo info = {0};
    AT(dvz_panel_frame_info(snapshot, &info));
    AT(info.struct_size == DVZ_STRUCT_SIZE(DvzPanelFrameInfo));
    AT(info.snapshot_id == dvz_panel_frame_id(snapshot));
    AT(info.snapshot_id != DVZ_ID_NONE);
    AT(info.figure_id == dvz_figure_id(figure));
    AT(info.panel_id == dvz_panel_id(panel));
    AT(info.view_id == view_state.view_id);
    AT(info.view_kind == DVZ_PANEL_VIEW_KIND_2D);
    AT(info.view_revision == view_state.revision);
    AT(info.logical_width_px == 400);
    AT(info.logical_height_px == 300);
    AC(info.device_scale_x, 1.0f, 1e-6f);
    AC(info.device_scale_y, 1.0f, 1e-6f);
    AC(info.user_scale, 1.0f, 1e-6f);
    AC(info.framebuffer_width_px, 400.0f, 1e-6f);
    AC(info.framebuffer_height_px, 300.0f, 1e-6f);

    AC(info.panel_rect_px.x, 100.0f, 1e-6f);
    AC(info.panel_rect_px.y, 60.0f, 1e-6f);
    AC(info.panel_rect_px.width, 200.0f, 1e-6f);
    AC(info.panel_rect_px.height, 180.0f, 1e-6f);
    DvzRect plot = {0};
    AT(dvz_panel_plot_rect_px(panel, &plot));
    AC(info.plot_rect_px.x, plot.x, 1e-6f);
    AC(info.plot_rect_px.y, plot.y, 1e-6f);
    AC(info.plot_rect_px.width, plot.width, 1e-6f);
    AC(info.plot_rect_px.height, plot.height, 1e-6f);
    AC(info.grid_clip_rect_px.x, plot.x, 1e-6f);
    AC(info.grid_clip_rect_px.y, plot.y, 1e-6f);
    AC(info.grid_clip_rect_px.width, plot.width, 1e-6f);
    AC(info.grid_clip_rect_px.height, plot.height, 1e-6f);
    AT(info.has_view2d);
    AT(info.has_valid_source_x);
    AT(info.has_valid_source_y);
    AT(info.has_valid_visible_x);
    AT(info.has_valid_visible_y);
    AC(info.source_data_x[0], -5.0, 1e-9);
    AC(info.source_data_x[1], 15.0, 1e-9);
    AC(info.source_data_y[0], 10.0, 1e-9);
    AC(info.source_data_y[1], -10.0, 1e-9);
    AT(info.diagnostics.count >= 1);

    const DvzId first_id = info.snapshot_id;
    const uint64_t first_revision = info.layout_revision;
    const uint64_t first_view_revision = info.view_revision;
    dvz_panel_frame_ref(snapshot);
    dvz_panel_frame_unref(snapshot);

    dvz_figure_resize(figure, 800, 300);
    DvzPanelFrameInfo frozen = {0};
    AT(dvz_panel_frame_info(snapshot, &frozen));
    AT(frozen.snapshot_id == first_id);
    AT(frozen.logical_width_px == 400);
    AT(frozen.layout_revision == first_revision);

    DvzPanelFrameSnapshot* after = dvz_panel_resolve_frame(panel);
    ANN(after);
    DvzPanelFrameInfo next = {0};
    AT(dvz_panel_frame_info(after, &next));
    AT(next.snapshot_id != first_id);
    AT(next.logical_width_px == 800);
    AT(next.layout_revision > first_revision);
    AT(next.panel_revision == next.layout_revision);
    AT(next.view_id == info.view_id);
    AT(next.view_kind == DVZ_PANEL_VIEW_KIND_2D);
    AT(next.view_revision == first_view_revision);
    DvzRect next_plot = {0};
    AT(dvz_panel_plot_rect_px(panel, &next_plot));
    AC(next.grid_clip_rect_px.x, next_plot.x, 1e-6f);
    AC(next.grid_clip_rect_px.y, next_plot.y, 1e-6f);
    AC(next.grid_clip_rect_px.width, next_plot.width, 1e-6f);
    AC(next.grid_clip_rect_px.height, next_plot.height, 1e-6f);

    dvz_panel_frame_unref(after);
    dvz_panel_frame_unref(snapshot);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_frame_snapshot_guide_layouts(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 420, 300, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzPanelAxes2DDesc axes = dvz_panel_axes_2d_desc();
    axes.x_label = "time";
    axes.y_label = "value";
    AT(dvz_panel_set_axes_2d(panel, &axes) == DVZ_OK);
    const double x_ticks[] = {0.0, 0.5, 1.0};
    const char* x_labels[] = {"zero", "half", "one"};
    DvzAxisTicks ticks = {
        DVZ_STRUCT_INIT_FIELDS(DvzAxisTicks),
        .count = 3,
        .values = x_ticks,
        .labels = x_labels,
    };
    AT(dvz_axis_set_ticks(dvz_panel_axis(panel, DVZ_DIM_X), &ticks) == DVZ_OK);

    DvzGuideLineDesc line_desc = dvz_guide_line_desc();
    line_desc.orientation = DVZ_GUIDE_ORIENTATION_HORIZONTAL;
    line_desc.value = 0.25;
    line_desc.label = "threshold";
    DvzGuideLine* line = dvz_guide_line(panel, &line_desc);
    ANN(line);

    DvzGuideSpanDesc span_desc = dvz_guide_span_desc();
    span_desc.orientation = DVZ_GUIDE_ORIENTATION_VERTICAL;
    span_desc.min_value = 0.20;
    span_desc.max_value = 0.40;
    span_desc.label = "window";
    DvzGuideSpan* span = dvz_guide_span(panel, &span_desc);
    ANN(span);

    _scene_prepare_axis_visuals(figure);
    _scene_prepare_guide_visuals(figure);

    DvzPanelFrameSnapshot* snapshot = dvz_panel_resolve_frame(panel);
    ANN(snapshot);
    DvzPanelFrameInfo info = {0};
    AT(dvz_panel_frame_info(snapshot, &info));

    const uint32_t guide_count = dvz_panel_frame_guide_count(snapshot);
    AT(guide_count > 0);
    const uint32_t contribution_count = dvz_panel_frame_contribution_count(snapshot);
    AT(contribution_count > 0);

    bool saw_grid = false;
    bool saw_axis_label = false;
    bool saw_tick_label = false;
    bool saw_line = false;
    bool saw_span = false;
    DvzGuideLayout hit_candidate = {0};
    for (uint32_t i = 0; i < guide_count; i++)
    {
        DvzGuideLayout layout = {0};
        AT(dvz_panel_frame_guide_layout(snapshot, i, &layout));
        AT(layout.struct_size == DVZ_STRUCT_SIZE(DvzGuideLayout));
        AT(layout.snapshot_id == info.snapshot_id);
        AT(layout.has_box);
        AT(layout.box_px.width > 0.0f);
        AT(layout.box_px.height > 0.0f);
        if (layout.role == DVZ_GUIDE_ROLE_AXIS_GRID)
            saw_grid = true;
        if (layout.role == DVZ_GUIDE_ROLE_AXIS_LABEL && strcmp(layout.label, "time") == 0)
            saw_axis_label = true;
        if (layout.role == DVZ_GUIDE_ROLE_AXIS_TICK_LABEL && strcmp(layout.label, "half") == 0)
        {
            saw_tick_label = true;
            hit_candidate = layout;
        }
        if (layout.role == DVZ_GUIDE_ROLE_GUIDE_LINE)
            saw_line = true;
        if (layout.role == DVZ_GUIDE_ROLE_GUIDE_SPAN)
            saw_span = true;
    }
    AT(saw_grid);
    AT(saw_axis_label);
    AT(saw_tick_label);
    AT(saw_line);
    AT(saw_span);

    DvzGuideHit hit = {0};
    AT(dvz_panel_frame_guide_hit(
        snapshot, hit_candidate.box_px.x + 0.5f * hit_candidate.box_px.width,
        hit_candidate.box_px.y + 0.5f * hit_candidate.box_px.height, &hit));
    AT(hit.struct_size == DVZ_STRUCT_SIZE(DvzGuideHit));
    AT(hit.hit);
    AT(hit.snapshot_id == info.snapshot_id);
    AT(hit.guide_id == hit_candidate.guide_id);

    DvzRenderedContribution contribution = {0};
    AT(dvz_panel_frame_contribution(snapshot, 0, &contribution));
    AT(contribution.struct_size == DVZ_STRUCT_SIZE(DvzRenderedContribution));
    AT(contribution.snapshot_id == info.snapshot_id);
    AT(contribution.visual_id != DVZ_ID_NONE);

    dvz_panel_frame_unref(snapshot);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_view3d_state_readback(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 320, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzPanelView3DDesc desc = dvz_panel_view3d_desc();
    AT(desc.struct_size == DVZ_STRUCT_SIZE(DvzPanelView3DDesc));
    desc.view.eye[0] = 1.0f;
    desc.view.eye[1] = 2.0f;
    desc.view.eye[2] = 3.0f;
    desc.view.target[0] = 0.25f;
    desc.view.target[1] = -0.50f;
    desc.view.target[2] = 0.75f;
    desc.view.up[0] = 0.0f;
    desc.view.up[1] = 0.0f;
    desc.view.up[2] = 1.0f;
    desc.projection.type = DVZ_CAMERA_ORTHOGRAPHIC;
    desc.projection.ortho_height = 4.0f;
    desc.projection.near_clip = 0.1f;
    desc.projection.far_clip = 50.0f;

    AT(dvz_panel_set_view3d_desc(panel, &desc) == 0);
    DvzPanelView3DState state = {0};
    AT(dvz_panel_view3d_state(panel, &state));
    AT(state.struct_size == DVZ_STRUCT_SIZE(DvzPanelView3DState));
    AT(state.enabled);
    AT(state.view_id != DVZ_ID_NONE);
    AT(state.revision > 0);
    AT(state.projection.type == DVZ_CAMERA_ORTHOGRAPHIC);
    AC(state.projection.ortho_height, 4.0f, 1e-6f);
    AC(state.projection.near_clip, 0.1f, 1e-6f);
    AC(state.projection.far_clip, 50.0f, 1e-6f);
    AC(state.view.eye[0], 1.0f, 1e-6f);
    AC(state.view.eye[1], 2.0f, 1e-6f);
    AC(state.view.eye[2], 3.0f, 1e-6f);
    AT(!state.has_explicit_orthographic_bounds);
    AT(fabsf(state.projection_matrix[0][0]) > 0.0f);
    AT(fabsf(state.projection_matrix[1][1]) > 0.0f);

    DvzPanelFrameSnapshot* snapshot = dvz_panel_resolve_frame(panel);
    ANN(snapshot);
    DvzPanelFrameInfo info = {0};
    AT(dvz_panel_frame_info(snapshot, &info));
    AT(info.view_kind == DVZ_PANEL_VIEW_KIND_3D);
    AT(info.view_id == state.view_id);
    AT(info.view_revision == state.revision);
    dvz_panel_frame_unref(snapshot);

    AT(dvz_camera_set_orthographic_bounds(
           dvz_panel_camera(panel), 4.0f, -4.0f, -2.0f, 2.0f, 0.2f, 60.0f) == 0);
    AT(dvz_panel_view3d_state(panel, &state));
    AT(state.has_explicit_orthographic_bounds);
    AC(state.orthographic_bounds[0], 4.0f, 1e-6f);
    AC(state.orthographic_bounds[1], -4.0f, 1e-6f);
    AC(state.orthographic_bounds[4], 0.2f, 1e-6f);
    AC(state.orthographic_bounds[5], 60.0f, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_adjacent_panels_plot_scissor_no_bleed(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    DvzPanel* left = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, &(DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});

    AT(dvz_panel_set_reserve(left, &(DvzPanelReserve){.right_px = 20.0f}) == DVZ_OK);
    AT(dvz_panel_set_reserve(right, &(DvzPanelReserve){.left_px = 20.0f}) == DVZ_OK);

    DvzRect left_plot = {0};
    DvzRect right_plot = {0};
    AT(dvz_panel_plot_rect_px(left, &left_plot));
    AT(dvz_panel_plot_rect_px(right, &right_plot));
    AC(left_plot.x, 0.0f, 1e-6f);
    AC(left_plot.width, 80.0f, 1e-6f);
    AC(right_plot.x, 120.0f, 1e-6f);
    AC(right_plot.width, 80.0f, 1e-6f);
    AT(left_plot.x + left_plot.width <= 100.0f);
    AT(right_plot.x >= 100.0f);

    // Oversized fixed-position points would cross the panel boundary without per-draw plot
    // scissors.
    float pos_left[3] = {1.0f, 0.0f, 0.0f};
    float pos_right[3] = {-1.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 255, 255};
    float size = 80.0f;

    DvzVisual* vl = dvz_point(scene, 0);
    DvzVisual* vr = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vl, "position", pos_left, 1) == 0);
    AT(dvz_visual_set_data(vl, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(vl, "size", &size, 1) == 0);
    AT(dvz_visual_set_data(vr, "position", pos_right, 1) == 0);
    AT(dvz_visual_set_data(vr, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(vr, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(left, vl, NULL) == 0);
    AT(dvz_panel_add_visual(right, vr, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool saw_left_plot_scissor = false;
    bool saw_right_plot_scissor = false;
    uint32_t draw_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            draw_count++;
        if (cmd->type != DVZ_DRP2_COMMAND_SET_SCISSOR)
            continue;

        if (fabsf(cmd->u.set_scissor.scissor[0] - left_plot.x) < 1e-5f &&
            fabsf(cmd->u.set_scissor.scissor[1] - left_plot.y) < 1e-5f &&
            fabsf(cmd->u.set_scissor.scissor[2] - left_plot.width) < 1e-5f &&
            fabsf(cmd->u.set_scissor.scissor[3] - left_plot.height) < 1e-5f)
        {
            saw_left_plot_scissor = true;
        }
        if (fabsf(cmd->u.set_scissor.scissor[0] - right_plot.x) < 1e-5f &&
            fabsf(cmd->u.set_scissor.scissor[1] - right_plot.y) < 1e-5f &&
            fabsf(cmd->u.set_scissor.scissor[2] - right_plot.width) < 1e-5f &&
            fabsf(cmd->u.set_scissor.scissor[3] - right_plot.height) < 1e-5f)
        {
            saw_right_plot_scissor = true;
        }
    }

    AT(draw_count == 2);
    AT(saw_left_plot_scissor);
    AT(saw_right_plot_scissor);

    _test_scene_stream_destroy(stream);
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
    AT(!dvz_visual_has_transform(visual));

    mat4 current = GLM_MAT4_IDENTITY_INIT;
    AT(dvz_visual_get_transform(visual, current) == 0);
    AC(current[0][0], 1.0f, 1e-6);
    AC(current[3][0], 0.0f, 1e-6);

    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    glm_translate(transform, (vec3){2.0f, 0.0f, 0.0f});
    AT(dvz_visual_set_transform(visual, transform) == 0);
    AT(dvz_visual_has_transform(visual));
    AT(dvz_visual_get_transform(visual, current) == 0);
    AC(current[3][0], 2.0f, 1e-6);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AC(bounds.min[0], 2.0, 1e-6);
    AC(bounds.max[0], 3.0, 1e-6);
    AC(bounds.min[1], -1.0, 1e-6);
    AC(bounds.max[1], +1.0, 1e-6);

    AT(dvz_visual_clear_transform(visual) == 0);
    AT(!dvz_visual_has_transform(visual));
    AT(dvz_visual_get_transform(visual, current) == 0);
    AC(current[3][0], 0.0f, 1e-6);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

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


int test_scene_visual_data_coord_space_tracks_panel_view2d_resize(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 100, 100, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

    DvzPanelView2DDesc view = dvz_panel_view2d_desc();
    view.aspect = DVZ_PANEL_VIEW2D_ASPECT_EQUAL;
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 5.0) == 0);
    AT(dvz_panel_set_view2d(panel, &view) == 0);

    vec3 pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor col[1] = {{255, 255, 255, 255}};
    float size[1] = {4.0f};
    DvzVisual* point = dvz_point(scene, 0);
    AT(dvz_visual_set_data(point, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(point, "color", col, 1) == 0);
    AT(dvz_visual_set_data(point, "size", size, 1) == 0);

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_VISUAL_COORD_DATA;
    AT(dvz_panel_add_visual(panel, point, &attach) == 0);

    DvzFramePlan* square = dvz_frame_plan("visual.data.coord.square", 0);
    ANN(square);
    AT(_scene_emit_panel_render(figure, 0, square, "figure_0"));
    const DvzFramePlanNode* square_render = dvz_frame_plan_node_get(square, 0);
    ANN(square_render);
    AT(square_render->u.render.visual_count == 1);
    AT(square_render->u.render.visual_has_mvp[0]);
    AC(square_render->u.render.visual_mvp[0].model[0][0], 0.2f, 1e-6);
    AC(square_render->u.render.visual_mvp[0].model[1][1], 0.2f, 1e-6);
    AC(square_render->u.render.visual_mvp[0].model[3][0], -1.0f, 1e-6);
    AC(square_render->u.render.visual_mvp[0].model[3][1], -0.5f, 1e-6);

    dvz_figure_resize(figure, 200, 100);
    DvzFramePlan* wide = dvz_frame_plan("visual.data.coord.wide", 0);
    ANN(wide);
    AT(_scene_emit_panel_render(figure, 0, wide, "figure_0"));
    const DvzFramePlanNode* wide_render = dvz_frame_plan_node_get(wide, 0);
    ANN(wide_render);
    AT(wide_render->u.render.visual_count == 1);
    AT(wide_render->u.render.visual_has_mvp[0]);
    AC(wide_render->u.render.visual_mvp[0].model[0][0], 0.4f, 1e-6);
    AC(wide_render->u.render.visual_mvp[0].model[1][1], 0.4f, 1e-6);
    AC(wide_render->u.render.visual_mvp[0].model[3][0], -2.0f, 1e-6);
    AC(wide_render->u.render.visual_mvp[0].model[3][1], -1.0f, 1e-6);

    dvz_frame_plan_destroy(wide);
    dvz_frame_plan_destroy(square);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_equal_aspect_view_and_panel_coord_spaces(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

    DvzPanelView2DDesc view = dvz_panel_view2d_desc();
    view.aspect = DVZ_PANEL_VIEW2D_ASPECT_EQUAL;
    AT(dvz_panel_set_view2d(panel, &view) == 0);

    float extent[4] = {0};
    AT(dvz_panel_view2d_extent(panel, extent));
    AC(extent[0], -2.0f, 1e-6);
    AC(extent[1], +2.0f, 1e-6);
    AC(extent[2], -1.0f, 1e-6);
    AC(extent[3], +1.0f, 1e-6);

    vec3 pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor col[1] = {{255, 255, 255, 255}};
    float size[1] = {4.0f};

    DvzVisual* view_point = dvz_point(scene, 0);
    AT(dvz_visual_set_data(view_point, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(view_point, "color", col, 1) == 0);
    AT(dvz_visual_set_data(view_point, "size", size, 1) == 0);
    DvzVisualAttachDesc view_attach = dvz_visual_attach_desc();
    view_attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    AT(dvz_panel_add_visual(panel, view_point, &view_attach) == 0);

    DvzVisual* panel_point = dvz_point(scene, 0);
    AT(dvz_visual_set_data(panel_point, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(panel_point, "color", col, 1) == 0);
    AT(dvz_visual_set_data(panel_point, "size", size, 1) == 0);
    DvzVisualAttachDesc panel_attach = dvz_visual_attach_desc();
    panel_attach.coord_space = DVZ_VISUAL_COORD_PANEL;
    AT(dvz_panel_add_visual(panel, panel_point, &panel_attach) == 0);

    vec3 pixel_pos[1] = {{100.0f, 50.0f, 0.0f}};
    DvzVisual* pixel_point = dvz_point(scene, 0);
    AT(dvz_visual_set_data(pixel_point, "position", pixel_pos, 1) == 0);
    AT(dvz_visual_set_data(pixel_point, "color", col, 1) == 0);
    AT(dvz_visual_set_data(pixel_point, "size", size, 1) == 0);
    DvzVisualAttachDesc pixel_attach = dvz_visual_attach_desc();
    pixel_attach.coord_space = DVZ_VISUAL_COORD_PANEL_PIXEL;
    pixel_attach.controller_mode = DVZ_CONTROLLER_FIXED;
    AT(dvz_panel_add_visual(panel, pixel_point, &pixel_attach) == 0);

    DvzFramePlan* plan = dvz_frame_plan("equal.aspect.view.panel", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(render->u.render.visual_count == 3);
    AC(render->u.render.apply_mvp.proj[0][0], 0.5f, 1e-6);
    AC(render->u.render.apply_mvp.proj[1][1], 1.0f, 1e-6);
    AT(!render->u.render.visual_has_mvp[0]);
    AT(render->u.render.visual_has_mvp[1]);
    AC(render->u.render.visual_mvp[1].proj[0][0], 1.0f, 1e-6);
    AC(render->u.render.visual_mvp[1].proj[1][1], 1.0f, 1e-6);
    AT(render->u.render.visual_has_mvp[2]);
    AC(render->u.render.visual_mvp[2].model[0][0], 2.0f / 200.0f, 1e-6);
    AC(render->u.render.visual_mvp[2].model[1][1], -2.0f / 100.0f, 1e-6);
    AC(render->u.render.visual_mvp[2].model[3][0], -1.0f, 1e-6);
    AC(render->u.render.visual_mvp[2].model[3][1], +1.0f, 1e-6);

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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
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


int test_scene_visual_local_transform_family_audit(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    const DvzVisualType types[] = {
        DVZ_VISUAL_TYPE_POINT,     DVZ_VISUAL_TYPE_PIXEL,  DVZ_VISUAL_TYPE_MARKER,
        DVZ_VISUAL_TYPE_SEGMENT,   DVZ_VISUAL_TYPE_PATH,   DVZ_VISUAL_TYPE_IMAGE,
        DVZ_VISUAL_TYPE_MESH,      DVZ_VISUAL_TYPE_PRIMITIVE,
        DVZ_VISUAL_TYPE_SPHERE,    DVZ_VISUAL_TYPE_SPLAT,  DVZ_VISUAL_TYPE_VECTOR,
    };

    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(types); i++)
    {
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
        DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
        ANN(panel);

        DvzVisual* visual = _local_transform_audit_visual(scene, types[i]);
        ANN(visual);

        mat4 transform = GLM_MAT4_IDENTITY_INIT;
        glm_translate(transform, (vec3){(float)i + 1.0f, 0.0f, 0.0f});
        AT(dvz_visual_set_transform(visual, transform) == 0);
        AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

        DvzFramePlan* plan = dvz_frame_plan("visual.local.family.audit", i);
        ANN(plan);
        AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));

        const DvzFramePlanNode* render = _first_render_with_visual(plan);
        ANN(render);
        AT(render->u.render.visual_has_mvp[0]);
        AC(render->u.render.visual_mvp[0].model[3][0], (float)i + 1.0f, 1e-6);

        dvz_frame_plan_destroy(plan);
        dvz_scene_destroy(scene);
    }

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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
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
                            &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_split_pass_visuals_emit_separate_common_mvp(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    /* Opaque and blended visuals are emitted in separate render nodes. Both become
     * local visual slot 0 in their pass, so common binding identity must include
     * stable visual/pass identity and not just the local slot index. */
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

    vec3 pos[1] = {{0.25f, 0.25f, 0.0f}};
    DvzColor opaque_col[1] = {{255, 0, 0, 255}};
    DvzColor blend_col[1] = {{0, 0, 255, 128}};
    float sz[1] = {8.0f};

    DvzVisual* opaque = dvz_point(scene, 0);
    DvzVisual* blended = dvz_point(scene, 0);
    ANN(opaque);
    ANN(blended);

    AT(dvz_visual_set_data(opaque, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_col, 1) == 0);
    AT(dvz_visual_set_data(opaque, "size", sz, 1) == 0);
    AT(dvz_visual_set_data(blended, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(blended, "color", blend_col, 1) == 0);
    AT(dvz_visual_set_data(blended, "size", sz, 1) == 0);
    AT(dvz_visual_set_alpha_mode(blended, DVZ_ALPHA_BLENDED) == 0);

    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, blended, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.supports_color_blending = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint64_t common_layout_id = _stream_scene_common_layout_id(stream);
    AT(common_layout_id != 0);

    uint32_t common_bg_count = 0;
    uint64_t mvp_buffers[2] = {0};
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
            continue;
        if (cmd->u.create_bind_group.bind_group_layout_id != common_layout_id)
            continue;
        AT(cmd->u.create_bind_group.entry_count == 2);
        const DvzDrp2BindGroupEntry* mvp = &cmd->u.create_bind_group.entries[0];
        const DvzDrp2BindGroupEntry* viewport = &cmd->u.create_bind_group.entries[1];
        AT(mvp->binding == DVZ_SCENE_SHADER_BINDING_COMMON_MVP);
        AT(mvp->binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER);
        AT(mvp->resource_kind == DVZ_DRP2_BINDING_RESOURCE_BUFFER);
        AT(mvp->size == sizeof(DvzMVP));
        AT(viewport->binding == DVZ_SCENE_SHADER_BINDING_COMMON_VIEWPORT);
        AT(viewport->binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER);
        AT(viewport->resource_kind == DVZ_DRP2_BINDING_RESOURCE_BUFFER);
        AT(viewport->size == sizeof(DvzSceneViewportUniform));
        AT(common_bg_count < DVZ_ARRAY_COUNT(mvp_buffers));
        mvp_buffers[common_bg_count++] = mvp->resource_id;
    }

    AT(common_bg_count == 2);
    AT(mvp_buffers[0] != 0);
    AT(mvp_buffers[1] != 0);
    AT(mvp_buffers[0] != mvp_buffers[1]);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_one_pass_per_panel(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});

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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
    DvzPanel* left = dvz_panel(figure, &(DvzPanelDesc){0, 0, 0.5f, 1});
    DvzPanel* right = dvz_panel(figure, &(DvzPanelDesc){0.5f, 0, 0.5f, 1});

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

    DvzVisualAttachDesc fixed = dvz_visual_attach_desc();
    fixed.controller_mode = DVZ_CONTROLLER_FIXED;
    fixed.coord_space = DVZ_VISUAL_COORD_VIEW;
    AT(dvz_panel_add_visual(left, vl, &fixed) == 0);
    AT(dvz_panel_add_visual(right, vr, &fixed) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
    AT(pass_count == 2);
    AT(draw_count == 2);
    AT(pipeline_count == 2);
    AT(bind_group_count == 2);
    AT(viewport_count == 2);
    AT(scissor_count == 2);

    _test_scene_stream_destroy(stream);
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
    DvzPanel* left = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, &(DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});

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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t pass_count = 0, clear_pass_count = 0, viewport_count = 0, scissor_count = 0;
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
            if (cmd->u.begin_render_pass.clear)
                clear_pass_count++;
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

    AT(pass_count == 2);
    AT(clear_pass_count == 1);
    AT(viewport_count == 2);
    AT(scissor_count == 2);

    _test_scene_stream_destroy(stream);
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
    DvzPanel* inset = dvz_panel(figure, &(DvzPanelDesc){0.55f, 0.55f, 0.35f, 0.35f});
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}
