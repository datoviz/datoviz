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

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define PANEL_FRAME_SNAPSHOT_EPS 1e-12



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPanelFrameSnapshot
{
    uint32_t ref_count;
    DvzPanelFrameInfo info;
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

    const uint64_t revision = panel->figure->frame_revision == 0 ? 1 : panel->figure->frame_revision;
    out->panel_revision = revision;
    out->layout_revision = revision;
    out->view_revision = revision;
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
        "coarse_frame_revisions: panel/layout/view/guide/visual revisions share the figure frame "
        "revision until dedicated retained objects land");
    _snapshot_add_diagnostic(
        &out->diagnostics,
        "guide_layout_snapshot_incomplete: first-class guide boxes and contribution identities are "
        "not part of the M182 core snapshot");
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
