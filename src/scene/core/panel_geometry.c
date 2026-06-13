/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene panel geometry                                                                         */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <float.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_controllers.h"
#include "_scene.h"
#include "annotation/axis_internal.h"
#include "datoviz/math/_cglm.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the panzoom payload bound to one panel dimension.
 *
 * @param panel the panel
 * @param dim the dimension
 * @return the borrowed panzoom payload, or NULL
 */
static DvzPanzoom* _scene_panel_bound_panzoom(const DvzPanel* panel, DvzDim dim)
{
    ANN(panel);
    if (dim > DVZ_DIM_Y)
        return NULL;
    DvzController* controller = panel->controllers[dim];
    if (controller != NULL && controller->type == DVZ_CONTROLLER_TYPE_PANZOOM)
        return controller->panzoom;
    if (controller == NULL)
        return panel->panzoom;
    return NULL;
}



/**
 * Compose a panel panzoom payload from its per-dimension bindings.
 *
 * @param panel the panel
 * @param out output panzoom payload
 * @return whether any panzoom state was found
 */
static bool _scene_panel_compose_panzoom(const DvzPanel* panel, DvzPanzoom* out)
{
    ANN(panel);
    ANN(out);
    DvzPanzoom* px = _scene_panel_bound_panzoom(panel, DVZ_DIM_X);
    DvzPanzoom* py = _scene_panel_bound_panzoom(panel, DVZ_DIM_Y);
    if (px == NULL && py == NULL)
        return false;

    dvz_memset(out, sizeof(DvzPanzoom), 0, sizeof(DvzPanzoom));
    out->zoom[0] = out->zoom[1] = 1.0f;
    out->zoom_center[0] = out->zoom_center[1] = 1.0f;
    out->zoom_min[0] = out->zoom_min[1] = 1e-3f;
    out->zoom_max[0] = out->zoom_max[1] = 1e+4f;
    out->has_zoom_limits = true;
    out->viewport_size[0] = 800.0f;
    out->viewport_size[1] = 600.0f;

    if (px != NULL)
    {
        out->pan[0] = px->pan[0];
        out->pan_center[0] = px->pan_center[0];
        out->zoom[0] = px->zoom[0];
        out->zoom_center[0] = px->zoom_center[0];
        out->zoom_min[0] = px->zoom_min[0];
        out->zoom_max[0] = px->zoom_max[0];
        out->pan_locked[0] = px->pan_locked[0];
        out->zoom_locked[0] = px->zoom_locked[0];
        out->flags |= px->flags & ~DVZ_PANZOOM_FLAGS_FIXED_Y;
        out->has_zoom_limits = out->has_zoom_limits || px->has_zoom_limits;
    }
    if (py != NULL)
    {
        out->pan[1] = py->pan[1];
        out->pan_center[1] = py->pan_center[1];
        out->zoom[1] = py->zoom[1];
        out->zoom_center[1] = py->zoom_center[1];
        out->zoom_min[1] = py->zoom_min[1];
        out->zoom_max[1] = py->zoom_max[1];
        out->pan_locked[1] = py->pan_locked[1];
        out->zoom_locked[1] = py->zoom_locked[1];
        out->flags |= py->flags & ~DVZ_PANZOOM_FLAGS_FIXED_X;
        out->has_zoom_limits = out->has_zoom_limits || py->has_zoom_limits;
    }
    return true;
}


/**
 * Initialize an identity MVP.
 *
 * @param out output MVP
 */
static void _scene_panel_identity_mvp(DvzMVP* out)
{
    ANN(out);
    glm_mat4_identity(out->model);
    glm_mat4_identity(out->view);
    glm_mat4_identity(out->proj);
    out->time  = 0.0f;
    out->flags = 0;
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

    if (panel->view_fit_enabled)
    {
        *out_x = panel->view_fit.x;
        *out_y = panel->view_fit.y;
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
    if (panel->view_fit_enabled)
    {
        const double pad = panel->view_fit.padding * fmax(x_span, y_span);
        xmin -= pad;
        xmax += pad;
        ymin -= pad;
        ymax += pad;
        x_span = xmax - xmin;
        y_span = ymax - ymin;
    }

    const bool equal_aspect =
        panel->view_fit_enabled && panel->view_fit.aspect == DVZ_PANEL_VIEW_ASPECT_EQUAL;
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
    if (!panel->view_fit_enabled)
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
 * Return the panel's visible panzoom extent from per-dimension bindings.
 *
 * @param panel the panel
 * @param out extent as xmin, xmax, ymin, ymax
 * @return whether a finite extent was written
 */
bool _scene_panel_panzoom_extent(const DvzPanel* panel, float out[4])
{
    ANN(panel);
    ANN(out);
    DvzPanelView2DResolved view = {0};
    if (!_scene_panel_view2d_resolve(panel, &view))
        return false;

    DvzPanzoom panzoom = {0};
    if (!_scene_panel_compose_panzoom(panel, &panzoom))
    {
        panzoom.zoom[0] = 1.0f;
        panzoom.zoom[1] = 1.0f;
    }
    DvzRect plot = {0};
    (void)dvz_panel_plot_rect_px(panel, &plot);
    DvzPanzoomResolved resolved = {0};
    if (!dvz_panzoom_resolve(
            &panzoom,
            &(DvzPanzoomEval){
                .base_extent = {
                    view.view_extent[0], view.view_extent[1], view.view_extent[2],
                    view.view_extent[3]},
                .viewport_width = plot.width,
                .viewport_height = plot.height,
            },
            &resolved))
    {
        return false;
    }
    out[0] = resolved.visible_extent[0];
    out[1] = resolved.visible_extent[1];
    out[2] = resolved.visible_extent[2];
    out[3] = resolved.visible_extent[3];
    return true;
}



/**
 * Build the per-panel apply MVP from the active controller state.
 *
 * @param panel the panel
 * @param out the destination MVP
 */
void _scene_panel_apply_mvp(const DvzPanel* panel, DvzMVP* out)
{
    ANN(panel);
    ANN(out);
    _scene_panel_identity_mvp(out);
    if (panel->camera != NULL)
    {
        dvz_camera_mvp(panel->camera, out);
    }
    else
    {
        DvzPanzoom panzoom = {0};
        if (!_scene_panel_compose_panzoom(panel, &panzoom))
        {
            if (!panel->view_fit_enabled)
                return;
            panzoom.zoom[0] = 1.0f;
            panzoom.zoom[1] = 1.0f;
        }
        DvzPanelView2DResolved view = {0};
        DvzRect plot = {0};
        (void)dvz_panel_plot_rect_px(panel, &plot);
        DvzPanzoomResolved resolved = {0};
        if (
            _scene_panel_view2d_resolve(panel, &view) &&
            dvz_panzoom_resolve(
                &panzoom,
                &(DvzPanzoomEval){
                    .base_extent = {
                        view.view_extent[0], view.view_extent[1], view.view_extent[2],
                        view.view_extent[3]},
                    .viewport_width = plot.width,
                    .viewport_height = plot.height,
                },
                &resolved))
        {
            *out = resolved.mvp;
        }
    }
    if (panel->arcball != NULL)
    {
        if (panel->camera != NULL)
            _dvz_arcball_view(panel->arcball, out->view);
        else
            _dvz_arcball_clear_view(panel->arcball);
        dvz_arcball_mvp(panel->arcball, out);
    }
}


/**
 * Build the old normalized-panel panzoom MVP for PANEL-coordinate attachments.
 *
 * @param panel the panel
 * @param out the destination MVP
 */
static void _scene_panel_apply_panel_mvp(const DvzPanel* panel, DvzMVP* out)
{
    ANN(panel);
    ANN(out);
    _scene_panel_identity_mvp(out);
    if (panel->camera != NULL)
    {
        dvz_camera_mvp(panel->camera, out);
        return;
    }

    DvzPanzoom panzoom = {0};
    if (_scene_panel_compose_panzoom(panel, &panzoom))
        dvz_panzoom_mvp(&panzoom, out);
}


/**
 * Build the affine DATA-space to VISUAL-space model matrix for one panel.
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


/**
 * Compose the effective MVP for one visual attachment.
 *
 * @param panel the panel owning the attachment
 * @param visual the visual
 * @param attach the panel attachment
 * @param apply_mvp optional precomputed panel APPLY MVP
 * @param out destination MVP
 * @return whether the MVP was resolved
 */
bool _scene_panel_attachment_mvp(
    const DvzPanel* panel, const DvzVisual* visual, const DvzPanelAttach* attach,
    const DvzMVP* apply_mvp, DvzMVP* out)
{
    ANN(panel);
    ANN(visual);
    ANN(attach);
    ANN(out);

    if (attach->coord_space == DVZ_COORD_PANEL)
        _scene_panel_apply_panel_mvp(panel, out);
    else if (apply_mvp != NULL)
        *out = *apply_mvp;
    else
        _scene_panel_apply_mvp(panel, out);

    if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
    {
        glm_mat4_identity(out->model);
        glm_mat4_identity(out->view);
        glm_mat4_identity(out->proj);
        out->flags = 0;
    }
    else if (attach->controller_mode == DVZ_CONTROLLER_APPLY_VIEW_PROJ)
    {
        glm_mat4_identity(out->model);
    }

    if (attach->coord_space == DVZ_COORD_DATA)
    {
        mat4 data = GLM_MAT4_IDENTITY_INIT;
        mat4 composed = GLM_MAT4_IDENTITY_INIT;
        if (!_scene_panel_data_model(panel, data))
            return false;
        glm_mat4_mul(out->model, data, composed);
        glm_mat4_copy(composed, out->model);
    }

    if (visual->has_local_transform)
    {
        mat4 local = GLM_MAT4_IDENTITY_INIT;
        mat4 composed = GLM_MAT4_IDENTITY_INIT;
        for (uint32_t col = 0; col < 4; col++)
        {
            for (uint32_t row = 0; row < 4; row++)
                local[col][row] = visual->local_transform[col][row];
        }
        glm_mat4_mul(out->model, local, composed);
        glm_mat4_copy(composed, out->model);
    }
    return true;
}



/**
 * Return a panel's pixel size, falling back to a conventional viewport size.
 *
 * @param panel the panel
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
void _scene_panel_pixel_size(const DvzPanel* panel, float* out_width, float* out_height)
{
    ANN(panel);
    ANN(out_width);
    ANN(out_height);
    float figure_width = panel->figure != NULL && panel->figure->width > 0 ?
                             (float)panel->figure->width :
                             800.0f;
    float figure_height = panel->figure != NULL && panel->figure->height > 0 ?
                              (float)panel->figure->height :
                              600.0f;
    float width = panel->desc.width * figure_width;
    float height = panel->desc.height * figure_height;
    *out_width = width > 0.0f ? width : 800.0f;
    *out_height = height > 0.0f ? height : 600.0f;
}



/**
 * Return a panel's pixel rectangle, falling back to a conventional viewport size.
 *
 * @param panel the panel
 * @param out_x output x origin in pixels
 * @param out_y output y origin in pixels
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
void _scene_panel_pixel_rect(
    const DvzPanel* panel, float* out_x, float* out_y, float* out_width, float* out_height)
{
    ANN(panel);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);
    float figure_width = panel->figure != NULL && panel->figure->width > 0 ?
                             (float)panel->figure->width :
                             800.0f;
    float figure_height = panel->figure != NULL && panel->figure->height > 0 ?
                              (float)panel->figure->height :
                              600.0f;
    *out_x = panel->desc.x * figure_width;
    *out_y = panel->desc.y * figure_height;
    _scene_panel_pixel_size(panel, out_width, out_height);
}


/**
 * Return a panel's padded inner pixel size.
 *
 * @param panel the panel
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
static void _scene_panel_inner_pixel_size(
    const DvzPanel* panel, float* out_width, float* out_height)
{
    ANN(panel);
    ANN(out_width);
    ANN(out_height);
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_size(panel, &width, &height);
    DvzPanelReserve padding = panel->padding;
    if (!_panel_padding_valid(panel, &padding))
        padding = (DvzPanelReserve){0};

    *out_width = width - padding.left_px - padding.right_px;
    *out_height = height - padding.top_px - padding.bottom_px;
}



/**
 * Return a panel's padded inner pixel rectangle.
 *
 * @param panel the panel
 * @param out_x output x origin in pixels
 * @param out_y output y origin in pixels
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
void _scene_panel_inner_pixel_rect(
    const DvzPanel* panel, float* out_x, float* out_y, float* out_width, float* out_height)
{
    ANN(panel);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(panel, &panel_x, &panel_y, &panel_width, &panel_height);
    DvzPanelReserve padding = panel->padding;
    if (!_panel_padding_valid(panel, &padding))
        padding = (DvzPanelReserve){0};

    *out_x = panel_x + padding.left_px;
    *out_y = panel_y + padding.top_px;
    *out_width = panel_width - padding.left_px - padding.right_px;
    *out_height = panel_height - padding.top_px - padding.bottom_px;
}



/**
 * Return a panel's plot rectangle in panel visual coordinates.
 *
 * @param panel the panel
 * @param out output rectangle as xmin, xmax, ymin, ymax
 */
void _scene_panel_plot_visual_rect(const DvzPanel* panel, float out[4])
{
    ANN(panel);
    ANN(out);
    DvzPanelReserve padding = panel->padding;
    if (!_panel_padding_valid(panel, &padding))
        padding = (DvzPanelReserve){0};
    DvzPanelReserve reserve = panel->reserve;
    if (!_panel_reserve_valid(panel, &reserve))
        reserve = (DvzPanelReserve){0};

    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_size(panel, &width, &height);
    const float left = width > 0.0f ? (padding.left_px + reserve.left_px) / width : 0.0f;
    const float right = width > 0.0f ? (padding.right_px + reserve.right_px) / width : 0.0f;
    const float top = height > 0.0f ? (padding.top_px + reserve.top_px) / height : 0.0f;
    const float bottom =
        height > 0.0f ? (padding.bottom_px + reserve.bottom_px) / height : 0.0f;

    out[0] = -1.0f + 2.0f * left;
    out[1] = +1.0f - 2.0f * right;
    out[2] = -1.0f + 2.0f * bottom;
    out[3] = +1.0f - 2.0f * top;

    if (out[1] <= out[0] || out[3] <= out[2])
    {
        out[0] = -1.0f;
        out[1] = +1.0f;
        out[2] = -1.0f;
        out[3] = +1.0f;
    }
}



/**
 * Return a panel's plot rectangle in figure pixels.
 *
 * @param panel the panel
 * @param out_x output x origin in pixels
 * @param out_y output y origin in pixels
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
void _scene_panel_plot_pixel_rect(
    const DvzPanel* panel, float* out_x, float* out_y, float* out_width, float* out_height)
{
    ANN(panel);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_inner_pixel_rect(panel, &panel_x, &panel_y, &panel_width, &panel_height);

    DvzPanelReserve reserve = panel->reserve;
    if (!_panel_reserve_valid(panel, &reserve))
        reserve = (DvzPanelReserve){0};

    *out_x = panel_x + reserve.left_px;
    *out_y = panel_y + reserve.top_px;
    *out_width = panel_width - reserve.left_px - reserve.right_px;
    *out_height = panel_height - reserve.top_px - reserve.bottom_px;
}



/**
 * Return a panel's plot rectangle in normalized figure coordinates.
 *
 * @param panel the panel
 * @return normalized plot rectangle
 */
DvzPanelDesc _scene_panel_plot_desc(const DvzPanel* panel)
{
    ANN(panel);

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_inner_pixel_rect(panel, &panel_x, &panel_y, &panel_width, &panel_height);
    DvzPanelReserve reserve = panel->reserve;
    if (!_panel_reserve_valid(panel, &reserve))
        reserve = (DvzPanelReserve){0};

    const float left = panel_width > 0.0f ? reserve.left_px / panel_width : 0.0f;
    const float right = panel_width > 0.0f ? reserve.right_px / panel_width : 0.0f;
    const float top = panel_height > 0.0f ? reserve.top_px / panel_height : 0.0f;
    const float bottom = panel_height > 0.0f ? reserve.bottom_px / panel_height : 0.0f;

    return (DvzPanelDesc){
        .x = panel->figure != NULL && panel->figure->width > 0 ?
                 (panel_x + reserve.left_px) / (float)panel->figure->width :
                 panel->desc.x + left * panel->desc.width,
        .y = panel->figure != NULL && panel->figure->height > 0 ?
                 (panel_y + reserve.top_px) / (float)panel->figure->height :
                 panel->desc.y + top * panel->desc.height,
        .width = panel->figure != NULL && panel->figure->width > 0 ?
                     (panel_width - reserve.left_px - reserve.right_px) /
                         (float)panel->figure->width :
                     (1.0f - left - right) * panel->desc.width,
        .height = panel->figure != NULL && panel->figure->height > 0 ?
                      (panel_height - reserve.top_px - reserve.bottom_px) /
                          (float)panel->figure->height :
                      (1.0f - top - bottom) * panel->desc.height,
    };
}
