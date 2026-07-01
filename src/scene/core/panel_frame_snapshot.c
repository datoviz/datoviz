/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene panel frame snapshot                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define PANEL_FRAME_SNAPSHOT_EPS 1e-12
#define PANEL_FRAME_MAX_GUIDES 512u
#define PANEL_FRAME_MAX_CONTRIBUTIONS 512u
#define PANEL_FRAME_TEXT_WIDTH_FACTOR 0.6f
#define PANEL_FRAME_MIN_TEXT_BOX_PX 4.0f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPanelFrameSnapshot
{
    uint32_t ref_count;
    DvzPanelFrameInfo info;
    uint32_t guide_count;
    DvzGuideLayout guides[PANEL_FRAME_MAX_GUIDES];
    uint32_t contribution_count;
    DvzRenderedContribution contributions[PANEL_FRAME_MAX_CONTRIBUTIONS];
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _snapshot_rect_valid(const DvzRect* rect)
{
    return rect != NULL && isfinite(rect->x) && isfinite(rect->y) && isfinite(rect->width) &&
           isfinite(rect->height) && rect->width > 0.0f && rect->height > 0.0f;
}


static bool _snapshot_interval_valid(double a, double b)
{
    return isfinite(a) && isfinite(b) && fabs(b - a) > PANEL_FRAME_SNAPSHOT_EPS;
}


static bool _snapshot_rect_contains(const DvzRect* rect, float x, float y)
{
    return _snapshot_rect_valid(rect) && isfinite(x) && isfinite(y) && x >= rect->x &&
           y >= rect->y && x <= rect->x + rect->width && y <= rect->y + rect->height;
}


static void _snapshot_copy_label(char out[DVZ_SCENE_LABEL_SIZE], const char* label)
{
    ANN(out);
    out[0] = '\0';
    if (label != NULL)
        strlcpy(out, label, DVZ_SCENE_LABEL_SIZE);
}


static DvzRect _snapshot_text_box(
    const DvzPanelFrameInfo* info, const float position[3], const float anchor[2], float size_px,
    const char* label)
{
    ANN(info);
    ANN(position);
    ANN(anchor);
    const float size =
        size_px > 0.0f && isfinite(size_px) ? size_px : PANEL_FRAME_MIN_TEXT_BOX_PX;
    const size_t len = label != NULL ? strlen(label) : 0;
    float width = fmaxf(PANEL_FRAME_MIN_TEXT_BOX_PX, (float)len * size * PANEL_FRAME_TEXT_WIDTH_FACTOR);
    float height = fmaxf(PANEL_FRAME_MIN_TEXT_BOX_PX, size * 1.2f);
    float x = info->panel_rect_px.x + position[0] - anchor[0] * width;
    float y = info->panel_rect_px.y + position[1] - anchor[1] * height;
    return (DvzRect){x, y, width, height};
}


static DvzRect _snapshot_rect_union(DvzRect a, DvzRect b)
{
    if (!_snapshot_rect_valid(&a))
        return b;
    if (!_snapshot_rect_valid(&b))
        return a;
    const float x0 = fminf(a.x, b.x);
    const float y0 = fminf(a.y, b.y);
    const float x1 = fmaxf(a.x + a.width, b.x + b.width);
    const float y1 = fmaxf(a.y + a.height, b.y + b.height);
    return (DvzRect){x0, y0, x1 - x0, y1 - y0};
}


static bool _snapshot_data_to_plot_x(const DvzPanelFrameInfo* info, double value, float* out)
{
    ANN(info);
    ANN(out);
    const double a = info->has_valid_visible_x ? info->visible_data_x[0] : info->source_data_x[0];
    const double b = info->has_valid_visible_x ? info->visible_data_x[1] : info->source_data_x[1];
    if (!_snapshot_interval_valid(a, b) || !isfinite(value))
        return false;
    const double t = (value - a) / (b - a);
    if (!isfinite(t))
        return false;
    *out = info->plot_rect_px.x + (float)t * info->plot_rect_px.width;
    return isfinite(*out);
}


static bool _snapshot_data_to_plot_y(const DvzPanelFrameInfo* info, double value, float* out)
{
    ANN(info);
    ANN(out);
    const double a = info->has_valid_visible_y ? info->visible_data_y[0] : info->source_data_y[0];
    const double b = info->has_valid_visible_y ? info->visible_data_y[1] : info->source_data_y[1];
    if (!_snapshot_interval_valid(a, b) || !isfinite(value))
        return false;
    const double t = (value - a) / (b - a);
    if (!isfinite(t))
        return false;
    *out = info->plot_rect_px.y + (1.0f - (float)t) * info->plot_rect_px.height;
    return isfinite(*out);
}


static DvzGuideLayout* _snapshot_push_guide(
    DvzPanelFrameSnapshot* snapshot, DvzId guide_id, DvzGuideKind kind, DvzGuideRole role,
    DvzGuidePart part, DvzRect box, const char* label)
{
    ANN(snapshot);
    if (snapshot->guide_count >= PANEL_FRAME_MAX_GUIDES || !_snapshot_rect_valid(&box))
        return NULL;
    DvzGuideLayout* guide = &snapshot->guides[snapshot->guide_count++];
    *guide = (DvzGuideLayout){DVZ_STRUCT_INIT_FIELDS(DvzGuideLayout)};
    guide->snapshot_id = snapshot->info.snapshot_id;
    guide->guide_id = guide_id;
    guide->kind = kind;
    guide->role = role;
    guide->part = part;
    guide->box_px = box;
    guide->has_box = true;
    guide->anchor_px[0] = box.x + 0.5f * box.width;
    guide->anchor_px[1] = box.y + 0.5f * box.height;
    guide->has_anchor = true;
    guide->item_index = snapshot->guide_count - 1;
    guide->has_item_index = true;
    _snapshot_copy_label(guide->label, label);
    return guide;
}


static void _snapshot_push_contribution(
    DvzPanelFrameSnapshot* snapshot, DvzId guide_id, DvzId visual_id, DvzGuideRole role,
    DvzGuidePart part, DvzRect box, const char* label)
{
    ANN(snapshot);
    if (snapshot->contribution_count >= PANEL_FRAME_MAX_CONTRIBUTIONS || visual_id == DVZ_ID_NONE)
        return;
    DvzRenderedContribution* contribution =
        &snapshot->contributions[snapshot->contribution_count++];
    *contribution = (DvzRenderedContribution){DVZ_STRUCT_INIT_FIELDS(DvzRenderedContribution)};
    contribution->snapshot_id = snapshot->info.snapshot_id;
    contribution->contribution_id = snapshot->contribution_count;
    contribution->guide_id = guide_id;
    contribution->visual_id = visual_id;
    contribution->kind = DVZ_RENDERED_CONTRIBUTION_VISUAL;
    contribution->role = role;
    contribution->part = part;
    contribution->box_px = box;
    _snapshot_copy_label(contribution->label, label);
}


static float _snapshot_inverse_panzoom_coord(
    const float extent[4], uint32_t lo_idx, uint32_t hi_idx, float value)
{
    ANN(extent);
    return 0.5f * (extent[lo_idx] + extent[hi_idx]) +
           0.5f * value * (extent[hi_idx] - extent[lo_idx]);
}


static double _snapshot_domain_visual_to_data(const DvzAxis* axis, float value)
{
    ANN(axis);
    if (!axis->domain_set)
        return (double)value;
    double t = 0.5 * ((double)value + 1.0);
    return axis->domain.min + t * (axis->domain.max - axis->domain.min);
}


static bool _snapshot_visible_domain_dim(
    const DvzPanel* panel, const DvzPanelView2DResolved* resolved, const float extent[4],
    DvzDim dim, double out[2])
{
    ANN(panel);
    ANN(resolved);
    ANN(extent);
    ANN(out);
    uint32_t lo_idx = dim == DVZ_DIM_X ? 0 : 2;
    uint32_t hi_idx = dim == DVZ_DIM_X ? 1 : 3;
    float a_view = _snapshot_inverse_panzoom_coord(extent, lo_idx, hi_idx, -1.0f);
    float b_view = _snapshot_inverse_panzoom_coord(extent, lo_idx, hi_idx, +1.0f);

    if (panel->view2d_enabled)
    {
        a_view = extent[lo_idx];
        b_view = extent[hi_idx];
        double data_min = dim == DVZ_DIM_X ? resolved->data_x[0] : resolved->data_y[0];
        double data_max = dim == DVZ_DIM_X ? resolved->data_x[1] : resolved->data_y[1];
        double view_min = (double)resolved->view_extent[lo_idx];
        double view_max = (double)resolved->view_extent[hi_idx];
        double scale = (view_max - view_min) / (data_max - data_min);
        double translate = view_min - scale * data_min;
        if (isfinite(scale) && fabs(scale) >= PANEL_FRAME_SNAPSHOT_EPS && isfinite(translate))
        {
            out[0] = ((double)a_view - translate) / scale;
            out[1] = ((double)b_view - translate) / scale;
            return _snapshot_interval_valid(out[0], out[1]);
        }
        return false;
    }

    const DvzAxis* axis = &panel->axes[(uint32_t)dim];
    out[0] = _snapshot_domain_visual_to_data(axis, a_view);
    out[1] = _snapshot_domain_visual_to_data(axis, b_view);
    return _snapshot_interval_valid(out[0], out[1]);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_panel_frame_snapshot(const DvzPanel* panel, DvzPanelFrameResolved* out)
{
    if (panel == NULL || out == NULL)
        return false;

    *out = (DvzPanelFrameResolved){0};
    glm_mat4_identity(out->data_to_view);
    out->plot_view[0] = -1.0f;
    out->plot_view[1] = +1.0f;
    out->plot_view[2] = -1.0f;
    out->plot_view[3] = +1.0f;
    out->view_extent[0] = -1.0f;
    out->view_extent[1] = +1.0f;
    out->view_extent[2] = -1.0f;
    out->view_extent[3] = +1.0f;
    out->controller_extent[0] = -1.0f;
    out->controller_extent[1] = +1.0f;
    out->controller_extent[2] = -1.0f;
    out->controller_extent[3] = +1.0f;

    _scene_panel_pixel_rect(
        panel, &out->panel_px.x, &out->panel_px.y, &out->panel_px.width,
        &out->panel_px.height);
    _scene_panel_inner_pixel_rect(
        panel, &out->inner_px.x, &out->inner_px.y, &out->inner_px.width,
        &out->inner_px.height);
    _scene_panel_plot_pixel_rect(
        panel, &out->plot_px.x, &out->plot_px.y, &out->plot_px.width, &out->plot_px.height);
    _scene_panel_plot_visual_rect(panel, out->plot_view);

    DvzPanelView2DResolved resolved = {0};
    bool has_resolved_view = _scene_panel_view2d_resolve(panel, &resolved);
    if (has_resolved_view)
    {
        out->has_view2d = panel->view2d_enabled;
        out->source_data_x[0] = resolved.data_x[0];
        out->source_data_x[1] = resolved.data_x[1];
        out->source_data_y[0] = resolved.data_y[0];
        out->source_data_y[1] = resolved.data_y[1];
        out->has_valid_source_x =
            _snapshot_interval_valid(out->source_data_x[0], out->source_data_x[1]);
        out->has_valid_source_y =
            _snapshot_interval_valid(out->source_data_y[0], out->source_data_y[1]);
        out->view_extent[0] = resolved.view_extent[0];
        out->view_extent[1] = resolved.view_extent[1];
        out->view_extent[2] = resolved.view_extent[2];
        out->view_extent[3] = resolved.view_extent[3];
        glm_mat4_copy(resolved.data_to_view, out->data_to_view);
    }

    if (!_scene_panel_panzoom_extent(panel, out->controller_extent) && has_resolved_view)
    {
        out->controller_extent[0] = resolved.view_extent[0];
        out->controller_extent[1] = resolved.view_extent[1];
        out->controller_extent[2] = resolved.view_extent[2];
        out->controller_extent[3] = resolved.view_extent[3];
    }

    if (has_resolved_view)
    {
        out->has_valid_visible_x = _snapshot_visible_domain_dim(
            panel, &resolved, out->controller_extent, DVZ_DIM_X, out->visible_data_x);
        out->has_valid_visible_y = _snapshot_visible_domain_dim(
            panel, &resolved, out->controller_extent, DVZ_DIM_Y, out->visible_data_y);
    }

    return _snapshot_rect_valid(&out->panel_px) && _snapshot_rect_valid(&out->plot_px);
}


static void _snapshot_add_diagnostic(DvzDiagnosticReport* report, const char* message)
{
    ANN(report);
    ANN(message);
    if (report->count >= DVZ_SCENE_MAX_DIAGNOSTICS)
        return;
    strlcpy(
        report->messages[report->count], message,
        sizeof(report->messages[report->count]));
    report->count++;
}


static void _snapshot_info_from_resolved(
    const DvzPanel* panel, const DvzPanelFrameResolved* resolved, DvzId snapshot_id,
    DvzPanelFrameInfo* out)
{
    ANN(panel);
    ANN(panel->figure);
    ANN(resolved);
    ANN(out);

    *out = (DvzPanelFrameInfo){DVZ_STRUCT_INIT_FIELDS(DvzPanelFrameInfo)};
    out->snapshot_id = snapshot_id;
    out->figure_id = dvz_figure_id(panel->figure);
    out->panel_id = dvz_panel_id(panel);
    out->view_kind = DVZ_PANEL_VIEW_KIND_NONE;
    if (panel->active_view_kind == DVZ_PANEL_VIEW_KIND_2D && panel->view2d_enabled)
    {
        out->view_kind = DVZ_PANEL_VIEW_KIND_2D;
        out->view_id = panel->view2d_id;
    }
    else if (panel->active_view_kind == DVZ_PANEL_VIEW_KIND_3D && panel->camera != NULL)
    {
        out->view_kind = DVZ_PANEL_VIEW_KIND_3D;
        out->view_id = panel->view3d_id;
    }

    const uint64_t revision = panel->figure->frame_revision == 0 ? 1 : panel->figure->frame_revision;
    out->panel_revision = revision;
    out->layout_revision = revision;
    if (out->view_kind == DVZ_PANEL_VIEW_KIND_2D)
        out->view_revision = panel->view2d_revision;
    else if (out->view_kind == DVZ_PANEL_VIEW_KIND_3D)
        out->view_revision = panel->view3d_revision;
    else
        out->view_revision = 0;
    out->guide_revision = revision;
    out->visual_revision = revision;

    out->logical_width_px = panel->figure->width;
    out->logical_height_px = panel->figure->height;
    out->device_scale_x = _scene_scale_or_one(panel->figure->device_scale_x);
    out->device_scale_y = _scene_scale_or_one(panel->figure->device_scale_y);
    out->user_scale = _scene_scale_or_one(panel->figure->user_scale);
    out->framebuffer_width_px = (float)out->logical_width_px * out->device_scale_x;
    out->framebuffer_height_px = (float)out->logical_height_px * out->device_scale_y;

    out->panel_rect_px = resolved->panel_px;
    out->inner_rect_px = resolved->inner_px;
    out->plot_rect_px = resolved->plot_px;
    out->grid_clip_rect_px = resolved->plot_px;
    for (uint32_t i = 0; i < 4; i++)
    {
        out->plot_view[i] = resolved->plot_view[i];
        out->view_extent[i] = resolved->view_extent[i];
        out->controller_extent[i] = resolved->controller_extent[i];
    }
    for (uint32_t i = 0; i < 2; i++)
    {
        out->source_data_x[i] = resolved->source_data_x[i];
        out->source_data_y[i] = resolved->source_data_y[i];
        out->visible_data_x[i] = resolved->visible_data_x[i];
        out->visible_data_y[i] = resolved->visible_data_y[i];
    }
    dvz_memcpy(out->data_to_view, sizeof(out->data_to_view), resolved->data_to_view,
               sizeof(resolved->data_to_view));
    out->has_view2d = resolved->has_view2d;
    out->has_valid_source_x = resolved->has_valid_source_x;
    out->has_valid_source_y = resolved->has_valid_source_y;
    out->has_valid_visible_x = resolved->has_valid_visible_x;
    out->has_valid_visible_y = resolved->has_valid_visible_y;
    _snapshot_add_diagnostic(
        &out->diagnostics,
        "coarse_frame_revisions: panel/layout/guide/visual revisions share the figure frame "
        "revision until dedicated retained objects land; view_revision is per active panel view");
    _snapshot_add_diagnostic(
        &out->diagnostics,
        "guide_layout_snapshot_first_slice: guide boxes use retained logical-pixel layout and "
        "coarse text extents until renderer glyph metrics are exported");
}


static void _snapshot_collect_axis(
    DvzPanelFrameSnapshot* snapshot, const DvzPanel* panel, const DvzAxis* axis)
{
    ANN(snapshot);
    ANN(panel);
    ANN(axis);
    if (!axis->enabled)
        return;

    const bool is_x = axis->dim == DVZ_DIM_X;
    const DvzGuideRole axis_role = is_x ? DVZ_GUIDE_ROLE_X_AXIS : DVZ_GUIDE_ROLE_Y_AXIS;
    DvzId axis_id = dvz_visual_id(axis->visual);
    if (axis_id == DVZ_ID_NONE)
        axis_id = dvz_visual_id(axis->text_visual);
    DvzGuideLayout* axis_layout = _snapshot_push_guide(
        snapshot, axis_id, DVZ_GUIDE_KIND_AXIS, axis_role, DVZ_GUIDE_PART_BOX,
        snapshot->info.plot_rect_px, axis->label);
    if (axis_layout != NULL)
    {
        _snapshot_push_contribution(
            snapshot, axis_id, dvz_visual_id(axis->visual), axis_role, DVZ_GUIDE_PART_LINE,
            snapshot->info.plot_rect_px, axis->label);
    }

    if (axis->style.show_grid)
    {
        DvzId grid_id = dvz_visual_id(axis->grid_visual);
        DvzGuideLayout* grid = _snapshot_push_guide(
            snapshot, grid_id != DVZ_ID_NONE ? grid_id : axis_id, DVZ_GUIDE_KIND_AXIS,
            DVZ_GUIDE_ROLE_AXIS_GRID, DVZ_GUIDE_PART_GRID, snapshot->info.grid_clip_rect_px,
            axis->label);
        if (grid != NULL)
            _snapshot_push_contribution(
                snapshot, grid->guide_id, grid_id, DVZ_GUIDE_ROLE_AXIS_GRID, DVZ_GUIDE_PART_GRID,
                snapshot->info.grid_clip_rect_px, axis->label);
    }

    for (uint32_t i = 0; i < axis->text_count; i++)
    {
        const char* label = axis->text_labels[i];
        const bool is_axis_label =
            axis->label[0] != '\0' && label != NULL && strcmp(label, axis->label) == 0;
        const DvzGuideRole role =
            is_axis_label ? DVZ_GUIDE_ROLE_AXIS_LABEL : DVZ_GUIDE_ROLE_AXIS_TICK_LABEL;
        DvzRect box = _snapshot_text_box(
            &snapshot->info, axis->text_positions[i], axis->text_anchors[i],
            axis->text_sizes[i], label);
        DvzGuideLayout* text = _snapshot_push_guide(
            snapshot, axis_id, DVZ_GUIDE_KIND_AXIS, role, DVZ_GUIDE_PART_TEXT, box, label);
        if (text == NULL)
            continue;
        text->anchor_px[0] = snapshot->info.panel_rect_px.x + axis->text_positions[i][0];
        text->anchor_px[1] = snapshot->info.panel_rect_px.y + axis->text_positions[i][1];
        text->item_index = i;
        if (i < axis->tick_count)
        {
            text->data_value = axis->ticks[i];
            text->has_data_value = isfinite(axis->ticks[i]);
        }
        _snapshot_push_contribution(
            snapshot, text->guide_id, dvz_visual_id(axis->text_visual), role, DVZ_GUIDE_PART_TEXT,
            box, label);
    }
}


static void _snapshot_collect_guide_lines(DvzPanelFrameSnapshot* snapshot, const DvzPanel* panel)
{
    ANN(snapshot);
    ANN(panel);
    DvzScene* scene = panel->figure != NULL ? panel->figure->scene : NULL;
    if (scene == NULL)
        return;

    for (uint32_t i = 0; i < scene->guide_line_count; i++)
    {
        DvzGuideLine* guide = &scene->guide_lines[i];
        if (!guide->active || guide->panel != panel)
            continue;
        const float thickness =
            fmaxf(1.0f, guide->desc.stroke_width_px > 0.0f ? guide->desc.stroke_width_px : 1.0f);
        DvzRect box = {0};
        if (guide->desc.orientation == DVZ_GUIDE_ORIENTATION_VERTICAL)
        {
            float x = 0.0f;
            if (!_snapshot_data_to_plot_x(&snapshot->info, guide->desc.value, &x))
                continue;
            box = (DvzRect){
                x - 0.5f * thickness, snapshot->info.plot_rect_px.y, thickness,
                snapshot->info.plot_rect_px.height};
        }
        else
        {
            float y = 0.0f;
            if (!_snapshot_data_to_plot_y(&snapshot->info, guide->desc.value, &y))
                continue;
            box = (DvzRect){
                snapshot->info.plot_rect_px.x, y - 0.5f * thickness,
                snapshot->info.plot_rect_px.width, thickness};
        }
        const DvzId guide_id = dvz_visual_id(guide->line_visual);
        DvzGuideLayout* layout = _snapshot_push_guide(
            snapshot, guide_id, DVZ_GUIDE_KIND_GUIDE_LINE, DVZ_GUIDE_ROLE_GUIDE_LINE,
            DVZ_GUIDE_PART_LINE, box, guide->desc.label);
        if (layout == NULL)
            continue;
        layout->data_value = guide->desc.value;
        layout->has_data_value = true;
        _snapshot_push_contribution(
            snapshot, guide_id, guide_id, DVZ_GUIDE_ROLE_GUIDE_LINE, DVZ_GUIDE_PART_LINE, box,
            guide->desc.label);
    }
}


static void _snapshot_collect_guide_spans(DvzPanelFrameSnapshot* snapshot, const DvzPanel* panel)
{
    ANN(snapshot);
    ANN(panel);
    DvzScene* scene = panel->figure != NULL ? panel->figure->scene : NULL;
    if (scene == NULL)
        return;

    for (uint32_t i = 0; i < scene->guide_span_count; i++)
    {
        DvzGuideSpan* span = &scene->guide_spans[i];
        if (!span->active || span->panel != panel)
            continue;
        DvzRect box = {0};
        if (span->desc.orientation == DVZ_GUIDE_ORIENTATION_VERTICAL)
        {
            float x0 = 0.0f, x1 = 0.0f;
            if (!_snapshot_data_to_plot_x(&snapshot->info, span->desc.min_value, &x0) ||
                !_snapshot_data_to_plot_x(&snapshot->info, span->desc.max_value, &x1))
                continue;
            box = (DvzRect){
                fminf(x0, x1), snapshot->info.plot_rect_px.y, fabsf(x1 - x0),
                snapshot->info.plot_rect_px.height};
        }
        else
        {
            float y0 = 0.0f, y1 = 0.0f;
            if (!_snapshot_data_to_plot_y(&snapshot->info, span->desc.min_value, &y0) ||
                !_snapshot_data_to_plot_y(&snapshot->info, span->desc.max_value, &y1))
                continue;
            box = (DvzRect){
                snapshot->info.plot_rect_px.x, fminf(y0, y1), snapshot->info.plot_rect_px.width,
                fabsf(y1 - y0)};
        }
        const DvzId guide_id = dvz_visual_id(span->fill_visual);
        DvzGuideLayout* layout = _snapshot_push_guide(
            snapshot, guide_id, DVZ_GUIDE_KIND_GUIDE_SPAN, DVZ_GUIDE_ROLE_GUIDE_SPAN,
            DVZ_GUIDE_PART_FILL, box, span->desc.label);
        if (layout == NULL)
            continue;
        layout->data_value = span->desc.min_value;
        layout->has_data_value = true;
        _snapshot_push_contribution(
            snapshot, guide_id, dvz_visual_id(span->fill_visual), DVZ_GUIDE_ROLE_GUIDE_SPAN,
            DVZ_GUIDE_PART_FILL, box, span->desc.label);
        _snapshot_push_contribution(
            snapshot, guide_id, dvz_visual_id(span->outline_visual), DVZ_GUIDE_ROLE_GUIDE_SPAN,
            DVZ_GUIDE_PART_OUTLINE, box, span->desc.label);
    }
}


static void _snapshot_collect_colorbars(DvzPanelFrameSnapshot* snapshot, const DvzPanel* panel)
{
    ANN(snapshot);
    ANN(panel);
    for (uint32_t i = 0; i < panel->colorbar_count; i++)
    {
        const DvzColorbar* colorbar = panel->colorbars[i];
        if (colorbar == NULL || colorbar->panel != panel)
            continue;

        DvzRect colorbar_box = {0};
        for (uint32_t j = 0; j < colorbar->text_count; j++)
        {
            const char* label = colorbar->text_labels[j];
            DvzRect box = _snapshot_text_box(
                &snapshot->info, colorbar->text_positions[j], colorbar->text_anchors[j],
                colorbar->text_sizes[j], label);
            colorbar_box = _snapshot_rect_union(colorbar_box, box);
            const bool is_title =
                colorbar->title[0] != '\0' && label != NULL && strcmp(label, colorbar->title) == 0;
            const DvzGuideRole role =
                is_title ? DVZ_GUIDE_ROLE_COLORBAR_TITLE : DVZ_GUIDE_ROLE_COLORBAR_TICK_LABEL;
            DvzGuideLayout* text = _snapshot_push_guide(
                snapshot, colorbar->id, DVZ_GUIDE_KIND_COLORBAR, role, DVZ_GUIDE_PART_TEXT, box,
                label);
            if (text == NULL)
                continue;
            text->anchor_px[0] = snapshot->info.panel_rect_px.x + colorbar->text_positions[j][0];
            text->anchor_px[1] = snapshot->info.panel_rect_px.y + colorbar->text_positions[j][1];
            text->item_index = j;
            if (j < colorbar->tick_count)
            {
                text->data_value = colorbar->ticks[j];
                text->has_data_value = isfinite(colorbar->ticks[j]);
            }
            _snapshot_push_contribution(
                snapshot, colorbar->id, dvz_visual_id(colorbar->text_visual), role,
                DVZ_GUIDE_PART_TEXT, box, label);
        }

        if (!_snapshot_rect_valid(&colorbar_box))
            colorbar_box = snapshot->info.panel_rect_px;
        DvzGuideLayout* layout = _snapshot_push_guide(
            snapshot, colorbar->id, DVZ_GUIDE_KIND_COLORBAR, DVZ_GUIDE_ROLE_COLORBAR,
            DVZ_GUIDE_PART_BOX, colorbar_box, colorbar->title);
        if (layout != NULL)
        {
            _snapshot_push_contribution(
                snapshot, colorbar->id, dvz_visual_id(colorbar->ramp_visual),
                DVZ_GUIDE_ROLE_COLORBAR_RAMP, DVZ_GUIDE_PART_RAMP, colorbar_box, colorbar->title);
            _snapshot_push_contribution(
                snapshot, colorbar->id, dvz_visual_id(colorbar->tick_visual),
                DVZ_GUIDE_ROLE_COLORBAR, DVZ_GUIDE_PART_TICK, colorbar_box, colorbar->title);
        }
    }
}


static void _snapshot_collect_legends(DvzPanelFrameSnapshot* snapshot, const DvzPanel* panel)
{
    ANN(snapshot);
    ANN(panel);
    for (uint32_t i = 0; i < panel->legend_count; i++)
    {
        const DvzLegend* legend = panel->legends[i];
        if (legend == NULL || legend->panel != panel)
            continue;

        DvzRect legend_box = {0};
        for (uint32_t j = 0; j < legend->text_count; j++)
        {
            const char* label = legend->text_labels[j];
            DvzRect box = _snapshot_text_box(
                &snapshot->info, legend->text_positions[j], legend->text_anchors[j],
                legend->text_sizes[j], label);
            legend_box = _snapshot_rect_union(legend_box, box);
            const bool is_title =
                legend->title[0] != '\0' && label != NULL && strcmp(label, legend->title) == 0;
            const DvzGuideRole role =
                is_title ? DVZ_GUIDE_ROLE_LEGEND_TITLE : DVZ_GUIDE_ROLE_LEGEND_ENTRY;
            DvzGuideLayout* text = _snapshot_push_guide(
                snapshot, legend->id, DVZ_GUIDE_KIND_LEGEND, role, DVZ_GUIDE_PART_TEXT, box,
                label);
            if (text == NULL)
                continue;
            text->anchor_px[0] = snapshot->info.panel_rect_px.x + legend->text_positions[j][0];
            text->anchor_px[1] = snapshot->info.panel_rect_px.y + legend->text_positions[j][1];
            text->item_index = j;
            _snapshot_push_contribution(
                snapshot, legend->id, dvz_visual_id(legend->text_visual), role,
                DVZ_GUIDE_PART_TEXT, box, label);
        }

        if (!_snapshot_rect_valid(&legend_box))
            legend_box = snapshot->info.panel_rect_px;
        DvzGuideLayout* layout = _snapshot_push_guide(
            snapshot, legend->id, DVZ_GUIDE_KIND_LEGEND, DVZ_GUIDE_ROLE_LEGEND,
            DVZ_GUIDE_PART_BOX, legend_box, legend->title);
        if (layout != NULL)
        {
            _snapshot_push_contribution(
                snapshot, legend->id, dvz_visual_id(legend->mark_visual),
                DVZ_GUIDE_ROLE_LEGEND_ENTRY, DVZ_GUIDE_PART_TICK, legend_box, legend->title);
        }
    }
}


static void _snapshot_collect_guides(DvzPanelFrameSnapshot* snapshot, const DvzPanel* panel)
{
    ANN(snapshot);
    ANN(panel);
    _snapshot_collect_axis(snapshot, panel, &panel->axes[DVZ_DIM_X]);
    _snapshot_collect_axis(snapshot, panel, &panel->axes[DVZ_DIM_Y]);
    _snapshot_collect_guide_lines(snapshot, panel);
    _snapshot_collect_guide_spans(snapshot, panel);
    _snapshot_collect_colorbars(snapshot, panel);
    _snapshot_collect_legends(snapshot, panel);
}


DvzPanelFrameSnapshot* dvz_panel_resolve_frame(DvzPanel* panel)
{
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzPanelFrameResolved resolved = {0};
    if (!_scene_panel_frame_snapshot(panel, &resolved))
        return NULL;
    DvzPanelFrameSnapshot* snapshot =
        (DvzPanelFrameSnapshot*)dvz_calloc(1, sizeof(DvzPanelFrameSnapshot));
    if (snapshot == NULL)
        return NULL;
    snapshot->ref_count = 1;
    _snapshot_info_from_resolved(
        panel, &resolved, _scene_next_id(panel->figure->scene), &snapshot->info);
    _snapshot_collect_guides(snapshot, panel);
    return snapshot;
}


DvzId dvz_panel_frame_id(const DvzPanelFrameSnapshot* snapshot)
{
    return snapshot != NULL ? snapshot->info.snapshot_id : DVZ_ID_NONE;
}


bool dvz_panel_frame_info(const DvzPanelFrameSnapshot* snapshot, DvzPanelFrameInfo* out)
{
    if (snapshot == NULL || out == NULL)
        return false;
    *out = snapshot->info;
    return true;
}


uint32_t dvz_panel_frame_guide_count(const DvzPanelFrameSnapshot* snapshot)
{
    return snapshot != NULL ? snapshot->guide_count : 0;
}


bool dvz_panel_frame_guide_layout(
    const DvzPanelFrameSnapshot* snapshot, uint32_t index, DvzGuideLayout* out)
{
    if (snapshot == NULL || out == NULL || index >= snapshot->guide_count)
        return false;
    *out = snapshot->guides[index];
    return true;
}


bool dvz_panel_frame_guide_hit(
    const DvzPanelFrameSnapshot* snapshot, float x_px, float y_px, DvzGuideHit* out)
{
    if (snapshot == NULL || out == NULL || !isfinite(x_px) || !isfinite(y_px))
        return false;
    for (uint32_t i = snapshot->guide_count; i > 0; i--)
    {
        const DvzGuideLayout* guide = &snapshot->guides[i - 1];
        if (!_snapshot_rect_contains(&guide->box_px, x_px, y_px))
            continue;
        *out = (DvzGuideHit){DVZ_STRUCT_INIT_FIELDS(DvzGuideHit)};
        out->snapshot_id = snapshot->info.snapshot_id;
        out->guide_id = guide->guide_id;
        out->kind = guide->kind;
        out->role = guide->role;
        out->part = guide->part;
        out->box_px = guide->box_px;
        out->point_px[0] = x_px;
        out->point_px[1] = y_px;
        out->data_value = guide->data_value;
        out->has_data_value = guide->has_data_value;
        out->item_index = guide->item_index;
        out->has_item_index = guide->has_item_index;
        out->hit = true;
        _snapshot_copy_label(out->label, guide->label);
        return true;
    }
    return false;
}


uint32_t dvz_panel_frame_contribution_count(const DvzPanelFrameSnapshot* snapshot)
{
    return snapshot != NULL ? snapshot->contribution_count : 0;
}


bool dvz_panel_frame_contribution(
    const DvzPanelFrameSnapshot* snapshot, uint32_t index, DvzRenderedContribution* out)
{
    if (snapshot == NULL || out == NULL || index >= snapshot->contribution_count)
        return false;
    *out = snapshot->contributions[index];
    return true;
}


void dvz_panel_frame_ref(DvzPanelFrameSnapshot* snapshot)
{
    if (snapshot == NULL)
        return;
    if (snapshot->ref_count < UINT32_MAX)
        snapshot->ref_count++;
}


void dvz_panel_frame_unref(DvzPanelFrameSnapshot* snapshot)
{
    if (snapshot == NULL)
        return;
    if (snapshot->ref_count > 1)
    {
        snapshot->ref_count--;
        return;
    }
    dvz_free(snapshot);
}
