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
    if (
        !isfinite(view->data_x.min) || !isfinite(view->data_x.max) ||
        !isfinite(view->data_y.min) || !isfinite(view->data_y.max) ||
        !isfinite(view->padding))
        return false;
    if (!(view->data_x.max > view->data_x.min) || !(view->data_y.max > view->data_y.min))
        return false;
    if (view->padding < 0.0)
        return false;
    return true;
}


void _scene_panel_view_dirty(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    DvzAxis* x_axis = _panel_axis_slot(panel, DVZ_DIM_X);
    DvzAxis* y_axis = _panel_axis_slot(panel, DVZ_DIM_Y);
    if (x_axis != NULL && x_axis->panel != NULL)
        _axis_mark_dirty(x_axis);
    if (y_axis != NULL && y_axis->panel != NULL)
        _axis_mark_dirty(y_axis);
    _scene_notify_request_frame(panel->figure);
}


/**
 * Return a finite positive data domain for one dimension.
 *
 * @param domain the source domain
 * @param out_min output minimum
 * @param out_max output maximum
 * @return whether the domain was valid
 */
static bool _scene_panel_domain_span(const DvzDataDomain* domain, double* out_min, double* out_max)
{
    ANN(domain);
    ANN(out_min);
    ANN(out_max);
    if (!isfinite(domain->min) || !isfinite(domain->max) || !(domain->max > domain->min))
        return false;
    *out_min = domain->min;
    *out_max = domain->max;
    return true;
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

    if (panel->view2d_enabled)
    {
        *out_x = panel->view2d.data_x;
        *out_y = panel->view2d.data_y;
        return true;
    }

    const DvzAxis* x_axis = &panel->axes[DVZ_DIM_X];
    const DvzAxis* y_axis = &panel->axes[DVZ_DIM_Y];
    if (x_axis->panel != NULL && x_axis->domain_set)
        *out_x = x_axis->domain;
    if (y_axis->panel != NULL && y_axis->domain_set)
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
        .data_x = {.min = -1.0, .max = +1.0},
        .data_y = {.min = -1.0, .max = +1.0},
        .padding = 0.0,
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
    {
        panel->view2d_enabled = false;
        _scene_panel_view_dirty(panel);
        return 0;
    }
    if (!_panel_view2d_validate(view))
        return -1;
    if (view->mode == DVZ_PANEL_VIEW2D_NONE)
    {
        panel->view2d_enabled = false;
        _scene_panel_view_dirty(panel);
        return 0;
    }
    panel->view2d = *view;
    panel->view2d_enabled = true;
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
        !_scene_panel_domain_span(&source_x, &xmin, &xmax) ||
        !_scene_panel_domain_span(&source_y, &ymin, &ymax))
    {
        return false;
    }

    double x_span = xmax - xmin;
    double y_span = ymax - ymin;
    if (panel->view2d_enabled)
    {
        const double pad = panel->view2d.padding * fmax(x_span, y_span);
        xmin -= pad;
        xmax += pad;
        ymin -= pad;
        ymax += pad;
        x_span = xmax - xmin;
        y_span = ymax - ymin;
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
            xmin = center - 0.5 * target_span;
            xmax = center + 0.5 * target_span;
            x_span = target_span;
        }
        else if (data_aspect > plot_aspect)
        {
            const double target_span = x_span / plot_aspect;
            const double center = 0.5 * (ymin + ymax);
            ymin = center - 0.5 * target_span;
            ymax = center + 0.5 * target_span;
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

    float data_extent[4] = {
        out->view_extent[0], out->view_extent[1], out->view_extent[2], out->view_extent[3]};
    if (!panel->view2d_enabled)
        _scene_panel_plot_visual_rect(panel, data_extent);

    const double data_x_span = (double)data_extent[1] - (double)data_extent[0];
    const double data_y_span = (double)data_extent[3] - (double)data_extent[2];
    const double sx = data_x_span / x_span;
    const double sy = data_y_span / y_span;
    const double tx = (double)data_extent[0] - sx * xmin;
    const double ty = (double)data_extent[2] - sy * ymin;
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
