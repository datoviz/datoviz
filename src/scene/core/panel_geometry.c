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

#include "_alloc.h"
#include "_assertions.h"
#include "_controllers.h"
#include "_scene.h"
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
    DvzPanzoom panzoom = {0};
    if (!_scene_panel_compose_panzoom(panel, &panzoom))
    {
        out[0] = -1.0f;
        out[1] = +1.0f;
        out[2] = -1.0f;
        out[3] = +1.0f;
        return true;
    }
    return dvz_panzoom_extent(&panzoom, out);
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
    glm_mat4_identity(out->model);
    glm_mat4_identity(out->view);
    glm_mat4_identity(out->proj);
    out->time  = 0.0f;
    out->flags = 0;
    if (panel->camera != NULL)
    {
        dvz_camera_mvp(panel->camera, out);
    }
    else
    {
        DvzPanzoom panzoom = {0};
        if (_scene_panel_compose_panzoom(panel, &panzoom))
            dvz_panzoom_mvp(&panzoom, out);
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

