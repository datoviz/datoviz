/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene panel view                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdbool.h>

#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "annotation/axis_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_PANEL_VIEW2D_KNOWN_FLAGS 0u
#define DVZ_PANEL_VIEW2D_DESC_KNOWN_FLAGS 0u
#define DVZ_PANEL_VIEW3D_DESC_KNOWN_FLAGS 0u


static uint64_t _panel_view_next_revision(uint64_t revision)
{
    return revision == UINT64_MAX ? 1 : revision + 1;
}


static bool _panel_view2d_validate(const DvzPanelView2D* view)
{
    if (view == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(view, DvzPanelView2D, DVZ_PANEL_VIEW2D_KNOWN_FLAGS))
    {
        log_error("invalid panel 2D view ABI");
        return false;
    }
    if (view->mode == DVZ_PANEL_VIEW2D_NONE)
        return true;
    if (view->mode != DVZ_PANEL_VIEW2D_CONTAIN)
        return false;
    if (view->aspect != DVZ_PANEL_VIEW2D_ASPECT_FREE &&
        view->aspect != DVZ_PANEL_VIEW2D_ASPECT_EQUAL)
        return false;
    if (!isfinite(view->padding))
        return false;
    if (view->padding < 0.0)
        return false;
    return true;
}


static bool _panel_view2d_domain_valid(const double domain[2])
{
    return domain != NULL && isfinite(domain[0]) && isfinite(domain[1]) &&
           fabs(domain[1] - domain[0]) > AXIS_EPS;
}


static bool _panel_view2d_desc_validate(const DvzPanelView2DDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzPanelView2DDesc, DVZ_PANEL_VIEW2D_DESC_KNOWN_FLAGS))
    {
        log_error("invalid panel 2D view descriptor ABI");
        return false;
    }
    if (desc->mode == DVZ_PANEL_VIEW2D_NONE)
        return true;
    if (desc->mode != DVZ_PANEL_VIEW2D_CONTAIN)
        return false;
    if (desc->aspect != DVZ_PANEL_VIEW2D_ASPECT_FREE &&
        desc->aspect != DVZ_PANEL_VIEW2D_ASPECT_EQUAL)
        return false;
    if (!isfinite(desc->padding) || desc->padding < 0.0)
        return false;
    if (desc->has_domain_x && !_panel_view2d_domain_valid(desc->domain_x))
        return false;
    if (desc->has_domain_y && !_panel_view2d_domain_valid(desc->domain_y))
        return false;
    return true;
}


static bool _panel_view3d_desc_validate(const DvzPanelView3DDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzPanelView3DDesc, DVZ_PANEL_VIEW3D_DESC_KNOWN_FLAGS))
    {
        log_error("invalid panel 3D view descriptor ABI");
        return false;
    }
    return true;
}


void _scene_panel_view_dirty(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    panel->view2d_revision = _panel_view_next_revision(panel->view2d_revision);
    DvzAxis* x_axis = _panel_axis_slot(panel, DVZ_DIM_X);
    DvzAxis* y_axis = _panel_axis_slot(panel, DVZ_DIM_Y);
    if (x_axis != NULL && x_axis->panel != NULL)
        _axis_mark_dirty(x_axis);
    if (y_axis != NULL && y_axis->panel != NULL)
        _axis_mark_dirty(y_axis);
    _scene_notify_request_frame(panel->figure);
}


void _scene_panel_view3d_dirty(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    panel->view3d_revision = _panel_view_next_revision(panel->view3d_revision);
    _scene_notify_request_frame(panel->figure);
}


/**
 * Return finite ordered data-domain endpoints for one dimension.
 *
 * @param domain the source domain
 * @param out_min output minimum
 * @param out_max output maximum
 * @return whether the domain was valid
 */
static bool _scene_panel_domain_endpoints(
    const DvzDataDomain* domain, double* out_min, double* out_max)
{
    ANN(domain);
    ANN(out_min);
    ANN(out_max);
    if (
        !isfinite(domain->min) || !isfinite(domain->max) ||
        fabs(domain->max - domain->min) <= AXIS_EPS)
        return false;
    *out_min = domain->min;
    *out_max = domain->max;
    return true;
}


static void _scene_panel_expand_ordered_domain(double* first, double* second, double padding)
{
    ANN(first);
    ANN(second);
    if (*second >= *first)
    {
        *first -= padding;
        *second += padding;
    }
    else
    {
        *first += padding;
        *second -= padding;
    }
}


static void _scene_panel_set_ordered_span(
    double center, double span, bool reversed, double* first, double* second)
{
    ANN(first);
    ANN(second);
    double lo = center - 0.5 * span;
    double hi = center + 0.5 * span;
    if (reversed)
    {
        *first = hi;
        *second = lo;
    }
    else
    {
        *first = lo;
        *second = hi;
    }
}


/**
 * Resolve the source data domains for one panel view.
 *
 * @param panel the panel
 * @param out_x output X domain
 * @param out_y output Y domain
 * @return whether the domains were resolved
 */
static bool _scene_panel_source_domains(
    const DvzPanel* panel, DvzDataDomain* out_x, DvzDataDomain* out_y)
{
    ANN(panel);
    ANN(out_x);
    ANN(out_y);
    *out_x = (DvzDataDomain){.min = -1.0, .max = +1.0};
    *out_y = (DvzDataDomain){.min = -1.0, .max = +1.0};

    const DvzAxis* x_axis = &panel->axes[DVZ_DIM_X];
    const DvzAxis* y_axis = &panel->axes[DVZ_DIM_Y];
    if (panel->view2d_domain_x_set)
        *out_x = panel->view2d_domain_x;
    else if (x_axis->panel != NULL && x_axis->domain_set)
        *out_x = x_axis->domain;
    if (panel->view2d_domain_y_set)
        *out_y = panel->view2d_domain_y;
    else if (y_axis->panel != NULL && y_axis->domain_set)
        *out_y = y_axis->domain;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default panel 2D view descriptor.
 *
 * @return panel 2D view descriptor
 */
DvzPanelView2D dvz_panel_view2d(void)
{
    return (DvzPanelView2D){
        DVZ_STRUCT_INIT_FIELDS(DvzPanelView2D),
        .mode = DVZ_PANEL_VIEW2D_CONTAIN,
        .aspect = DVZ_PANEL_VIEW2D_ASPECT_FREE,
        .padding = 0.0,
    };
}


DvzPanelView2DDesc dvz_panel_view2d_desc(void)
{
    return (DvzPanelView2DDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzPanelView2DDesc),
        .mode = DVZ_PANEL_VIEW2D_CONTAIN,
        .aspect = DVZ_PANEL_VIEW2D_ASPECT_FREE,
        .padding = 0.0,
        .domain_x = {-1.0, +1.0},
        .domain_y = {-1.0, +1.0},
        .has_domain_x = false,
        .has_domain_y = false,
    };
}


/**
 * Set a panel 2D view policy.
 *
 * @param panel the panel
 * @param view panel 2D view descriptor; NULL clears the view policy
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_view2d(DvzPanel* panel, const DvzPanelView2D* view)
{
    if (panel == NULL)
        return -1;
    if (view == NULL)
        return dvz_panel_set_view2d_desc(panel, NULL);
    if (!_panel_view2d_validate(view))
        return -1;
    if (view->mode == DVZ_PANEL_VIEW2D_NONE)
        return dvz_panel_set_view2d_desc(panel, NULL);

    DvzPanelView2DDesc desc = dvz_panel_view2d_desc();
    desc.mode = view->mode;
    desc.aspect = view->aspect;
    desc.padding = view->padding;
    return dvz_panel_set_view2d_desc(panel, &desc);
}


int dvz_panel_set_view2d_desc(DvzPanel* panel, const DvzPanelView2DDesc* desc)
{
    if (panel == NULL)
        return -1;
    if (desc == NULL)
    {
        panel->view2d_enabled = false;
        if (panel->active_view_kind == DVZ_PANEL_VIEW_KIND_2D)
            panel->active_view_kind = DVZ_PANEL_VIEW_KIND_NONE;
        _scene_panel_view_dirty(panel);
        return 0;
    }
    if (!_panel_view2d_desc_validate(desc))
        return -1;
    if (desc->mode == DVZ_PANEL_VIEW2D_NONE)
        return dvz_panel_set_view2d_desc(panel, NULL);

    panel->view2d = (DvzPanelView2D){
        DVZ_STRUCT_INIT_FIELDS(DvzPanelView2D),
        .mode = desc->mode,
        .aspect = desc->aspect,
        .padding = desc->padding,
    };
    panel->view2d_enabled = true;
    panel->active_view_kind = DVZ_PANEL_VIEW_KIND_2D;
    if (desc->has_domain_x &&
        dvz_panel_set_domain(panel, DVZ_DIM_X, desc->domain_x[0], desc->domain_x[1]) != 0)
    {
        return -1;
    }
    if (desc->has_domain_y &&
        dvz_panel_set_domain(panel, DVZ_DIM_Y, desc->domain_y[0], desc->domain_y[1]) != 0)
    {
        return -1;
    }
    _scene_panel_view_dirty(panel);
    return 0;
}


/**
 * Clear a panel 2D view policy without changing the current axis domains.
 *
 * @param panel the panel
 */
void dvz_panel_clear_view2d(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    panel->view2d_enabled = false;
    if (panel->active_view_kind == DVZ_PANEL_VIEW_KIND_2D)
        panel->active_view_kind = DVZ_PANEL_VIEW_KIND_NONE;
    _scene_panel_view_dirty(panel);
}


/**
 * Return the current resolved panel VIEW extent before panzoom.
 *
 * @param panel the panel
 * @param out output extent as xmin, xmax, ymin, ymax
 * @return whether the extent was written
 */
bool dvz_panel_view2d_extent(DvzPanel* panel, float out[4])
{
    if (panel == NULL || out == NULL)
        return false;
    DvzPanelView2DResolved resolved = {0};
    if (!_scene_panel_view2d_resolve(panel, &resolved))
        return false;
    out[0] = resolved.view_extent[0];
    out[1] = resolved.view_extent[1];
    out[2] = resolved.view_extent[2];
    out[3] = resolved.view_extent[3];
    return true;
}


bool dvz_panel_view2d_state(const DvzPanel* panel, DvzPanelView2DState* out)
{
    if (panel == NULL || out == NULL)
        return false;
    DvzPanelView2DResolved resolved = {0};
    if (!_scene_panel_view2d_resolve(panel, &resolved))
        return false;
    *out = (DvzPanelView2DState){
        DVZ_STRUCT_INIT_FIELDS(DvzPanelView2DState),
        .view_id = panel->view2d_id,
        .revision = panel->view2d_revision,
        .enabled = panel->view2d_enabled,
        .mode = panel->view2d.mode,
        .aspect = panel->view2d.aspect,
        .padding = panel->view2d.padding,
        .has_domain_x = panel->view2d_domain_x_set,
        .has_domain_y = panel->view2d_domain_y_set,
    };
    DvzDataDomain source_x = {.min = resolved.data_x[0], .max = resolved.data_x[1]};
    DvzDataDomain source_y = {.min = resolved.data_y[0], .max = resolved.data_y[1]};
    out->has_valid_source_x =
        _scene_panel_domain_endpoints(&source_x, &out->domain_x[0], &out->domain_x[1]);
    out->has_valid_source_y =
        _scene_panel_domain_endpoints(&source_y, &out->domain_y[0], &out->domain_y[1]);
    for (uint32_t i = 0; i < 4; i++)
        out->view_extent[i] = resolved.view_extent[i];
    glm_mat4_copy(resolved.data_to_view, out->data_to_view);
    return true;
}


DvzPanelView3DDesc dvz_panel_view3d_desc(void)
{
    DvzPanelView3DDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzPanelView3DDesc),
    };
    desc.camera = dvz_camera_desc();
    return desc;
}


int dvz_panel_set_view3d_desc(DvzPanel* panel, const DvzPanelView3DDesc* desc)
{
    if (panel == NULL)
        return -1;
    if (desc == NULL)
    {
        if (panel->camera != NULL)
        {
            dvz_camera_destroy(panel->camera);
            panel->camera = NULL;
        }
        if (panel->active_view_kind == DVZ_PANEL_VIEW_KIND_3D)
            panel->active_view_kind = DVZ_PANEL_VIEW_KIND_NONE;
        _scene_panel_view3d_dirty(panel);
        return 0;
    }
    if (!_panel_view3d_desc_validate(desc))
        return -1;
    if (dvz_panel_set_camera(panel, &desc->camera) == NULL)
        return -1;
    panel->active_view_kind = DVZ_PANEL_VIEW_KIND_3D;
    return 0;
}


bool dvz_panel_view3d_state(DvzPanel* panel, DvzPanelView3DState* out)
{
    if (panel == NULL || out == NULL)
        return false;
    *out = (DvzPanelView3DState){
        DVZ_STRUCT_INIT_FIELDS(DvzPanelView3DState),
        .view_id = panel->view3d_id,
        .revision = panel->view3d_revision,
        .enabled = panel->camera != NULL,
    };
    glm_mat4_identity(out->mvp.model);
    glm_mat4_identity(out->mvp.view);
    glm_mat4_identity(out->mvp.proj);
    if (panel->camera == NULL)
        return true;

    dvz_camera_get_view(panel->camera, &out->view);
    dvz_camera_get_projection(panel->camera, &out->projection);
    if (dvz_camera_get_orthographic_bounds(
            panel->camera, &out->orthographic_bounds[0], &out->orthographic_bounds[1],
            &out->orthographic_bounds[2], &out->orthographic_bounds[3],
            &out->orthographic_bounds[4], &out->orthographic_bounds[5]) == 0)
    {
        out->has_explicit_orthographic_bounds = true;
    }
    dvz_camera_mvp(panel->camera, &out->mvp);
    return true;
}


/**
 * Resolve one panel's 2D VIEW and DATA mapping state.
 *
 * @param panel the panel
 * @param out output resolved state
 * @return whether the view was resolved
 */
bool _scene_panel_view2d_resolve(const DvzPanel* panel, DvzPanelView2DResolved* out)
{
    ANN(panel);
    ANN(out);
    out->view_extent[0] = -1.0f;
    out->view_extent[1] = +1.0f;
    out->view_extent[2] = -1.0f;
    out->view_extent[3] = +1.0f;
    out->data_x[0] = -1.0;
    out->data_x[1] = +1.0;
    out->data_y[0] = -1.0;
    out->data_y[1] = +1.0;
    glm_mat4_identity(out->data_to_view);

    DvzDataDomain source_x = {0};
    DvzDataDomain source_y = {0};
    if (!_scene_panel_source_domains(panel, &source_x, &source_y))
        return false;

    double xmin = 0.0, xmax = 0.0, ymin = 0.0, ymax = 0.0;
    if (
        !_scene_panel_domain_endpoints(&source_x, &xmin, &xmax) ||
        !_scene_panel_domain_endpoints(&source_y, &ymin, &ymax))
    {
        return false;
    }

    const bool x_reversed = xmax < xmin;
    const bool y_reversed = ymax < ymin;
    double x_span = fabs(xmax - xmin);
    double y_span = fabs(ymax - ymin);
    if (panel->view2d_enabled)
    {
        const double pad = panel->view2d.padding * fmax(x_span, y_span);
        _scene_panel_expand_ordered_domain(&xmin, &xmax, pad);
        _scene_panel_expand_ordered_domain(&ymin, &ymax, pad);
        x_span = fabs(xmax - xmin);
        y_span = fabs(ymax - ymin);
    }

    const bool equal_aspect =
        panel->view2d_enabled && panel->view2d.aspect == DVZ_PANEL_VIEW2D_ASPECT_EQUAL;
    if (equal_aspect)
    {
        DvzRect plot = {0};
        if (!dvz_panel_plot_rect_px(panel, &plot) || plot.width <= 0.0f || plot.height <= 0.0f)
            return false;
        const double plot_aspect = (double)plot.width / (double)plot.height;
        const double data_aspect = x_span / y_span;
        if (!isfinite(plot_aspect) || !(plot_aspect > 0.0) || !isfinite(data_aspect) ||
            !(data_aspect > 0.0))
        {
            return false;
        }
        if (data_aspect < plot_aspect)
        {
            const double target_span = y_span * plot_aspect;
            const double center = 0.5 * (xmin + xmax);
            _scene_panel_set_ordered_span(center, target_span, x_reversed, &xmin, &xmax);
            x_span = target_span;
        }
        else if (data_aspect > plot_aspect)
        {
            const double target_span = x_span / plot_aspect;
            const double center = 0.5 * (ymin + ymax);
            _scene_panel_set_ordered_span(center, target_span, y_reversed, &ymin, &ymax);
            y_span = target_span;
        }

        if (plot_aspect >= 1.0)
        {
            out->view_extent[0] = (float)-plot_aspect;
            out->view_extent[1] = (float)+plot_aspect;
            out->view_extent[2] = -1.0f;
            out->view_extent[3] = +1.0f;
        }
        else
        {
            out->view_extent[0] = -1.0f;
            out->view_extent[1] = +1.0f;
            out->view_extent[2] = (float)(-1.0 / plot_aspect);
            out->view_extent[3] = (float)(+1.0 / plot_aspect);
        }
    }

    out->data_x[0] = xmin;
    out->data_x[1] = xmax;
    out->data_y[0] = ymin;
    out->data_y[1] = ymax;

    const double data_x_span = (double)out->view_extent[1] - (double)out->view_extent[0];
    const double data_y_span = (double)out->view_extent[3] - (double)out->view_extent[2];
    const double sx = data_x_span / (xmax - xmin);
    const double sy = data_y_span / (ymax - ymin);
    const double tx = (double)out->view_extent[0] - sx * xmin;
    const double ty = (double)out->view_extent[2] - sy * ymin;
    if (
        !isfinite(sx) || !isfinite(sy) || !isfinite(tx) || !isfinite(ty) ||
        fabs(sx) > (double)FLT_MAX || fabs(sy) > (double)FLT_MAX ||
        fabs(tx) > (double)FLT_MAX || fabs(ty) > (double)FLT_MAX)
    {
        return false;
    }

    out->data_to_view[0][0] = (float)sx;
    out->data_to_view[1][1] = (float)sy;
    out->data_to_view[3][0] = (float)tx;
    out->data_to_view[3][1] = (float)ty;
    return true;
}


/**
 * Build the affine DATA-space to VIEW-space model matrix for one panel.
 *
 * @param panel the panel
 * @param out destination matrix
 * @return whether the transform was resolved
 */
bool _scene_panel_data_model(const DvzPanel* panel, mat4 out)
{
    ANN(panel);
    ANN(out);
    glm_mat4_identity(out);
    DvzPanelView2DResolved view = {0};
    if (!_scene_panel_view2d_resolve(panel, &view))
        return false;
    glm_mat4_copy(view.data_to_view, out);
    return true;
}
