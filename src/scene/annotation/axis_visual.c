/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene 2D axis generated visuals                                                              */
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
#include "_compat.h"
#include "axis_internal.h"
#include "prepare_internal.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a major tick length in visual units, with the style length interpreted as pixels.
 *
 * @param axis the axis
 * @param length_px length in pixels
 * @return major tick length in visual units
 */
static float _axis_tick_length(const DvzAxis* axis, float length_px)
{
    ANN(axis);
    if (!(length_px > 0.0f) || !isfinite(length_px))
        return 0.0f;
    if (axis->panel == NULL)
        return 0.0f;
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(axis->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    float span = axis->dim == DVZ_DIM_X ? panel_height : panel_width;
    if (!(span > 0.0f) || !isfinite(span))
        return 0.0f;
    return 2.0f * length_px / span;
}



/**
 * Return the user-controlled scale for one axis' panel.
 *
 * @param axis the axis
 * @return positive finite user scale
 */
static float _axis_user_scale(const DvzAxis* axis)
{
    ANN(axis);
    if (axis->panel == NULL || axis->panel->figure == NULL)
        return 1.0f;
    return _scene_scale_or_one(axis->panel->figure->user_scale);
}



/**
 * Return the visual-space size of one pixel for one screen axis.
 *
 * @param axis the axis
 * @param dim the visual dimension
 * @return visual-space pixel size
 */
static float _axis_visual_pixel_size(const DvzAxis* axis, DvzDim dim)
{
    ANN(axis);
    if (axis->panel == NULL)
        return 0.0f;
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(axis->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    float span = dim == DVZ_DIM_X ? panel_width : panel_height;
    if (!(span > 0.0f) || !isfinite(span))
        return 0.0f;
    return 2.0f / span;
}


/**
 * Return the plot-viewport visual-space size of one pixel for one screen axis.
 *
 * @param axis the axis
 * @param dim the visual dimension
 * @return plot visual-space pixel size
 */
static float _axis_plot_pixel_size(const DvzAxis* axis, DvzDim dim)
{
    ANN(axis);
    if (axis->panel == NULL)
        return 0.0f;
    DvzRect plot_px = {0};
    if (!dvz_panel_plot_rect_px(axis->panel, &plot_px))
        return 0.0f;
    float span = dim == DVZ_DIM_X ? plot_px.width : plot_px.height;
    if (!(span > 0.0f) || !isfinite(span))
        return 0.0f;
    return 2.0f / span;
}



/**
 * Return the pixel phase that centers a line with stable coverage.
 *
 * @param width_px line width in pixels
 * @return 0 for even-width lines, 0.5 for odd-width lines
 */
static float _axis_line_pixel_phase(float width_px)
{
    if (!(width_px > 0.0f) || !isfinite(width_px))
        return 0.5f;
    uint32_t rounded = (uint32_t)fmaxf(1.0f, floorf(width_px + 0.5f));
    return (rounded % 2u) == 0u ? 0.0f : 0.5f;
}


/**
 * Snap one displayed visual coordinate to a width-aware pixel center.
 *
 * @param axis the axis
 * @param value displayed visual coordinate
 * @param dim the visual dimension
 * @param width_px line width in pixels
 * @return snapped displayed visual coordinate
 */
static float
_axis_snap_visual_pixel_center_basis(
    const DvzAxis* axis, float value, DvzDim dim, float width_px, bool plot_pixels)
{
    ANN(axis);
    if (axis->panel == NULL)
        return value;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    DvzRect plot_px = {0};
    if (plot_pixels && dvz_panel_plot_rect_px(axis->panel, &plot_px))
    {
        panel_width = plot_px.width;
        panel_height = plot_px.height;
    }
    else
    {
        float panel_x = 0.0f;
        float panel_y = 0.0f;
        _scene_panel_pixel_rect(axis->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    }
    float span = dim == DVZ_DIM_X ? panel_width : panel_height;
    if (!(span > 0.0f) || !isfinite(span))
        return value;
    float pixel = (value + 1.0f) * 0.5f * span;
    float phase = _axis_line_pixel_phase(width_px);
    float snapped = floorf(pixel - phase + 0.5f) + phase;
    return 2.0f * snapped / span - 1.0f;
}


/**
 * Snap one displayed visual coordinate to a panel-pixel center.
 *
 * @param axis the axis
 * @param value displayed visual coordinate
 * @param dim the visual dimension
 * @param width_px line width in pixels
 * @return snapped displayed visual coordinate
 */
static float
_axis_snap_visual_pixel_center(const DvzAxis* axis, float value, DvzDim dim, float width_px)
{
    return _axis_snap_visual_pixel_center_basis(axis, value, dim, width_px, false);
}


/**
 * Snap one source visual coordinate after panzoom projection.
 *
 * @param axis the axis
 * @param extent full-panel inverse panzoom extent as xmin, xmax, ymin, ymax
 * @param dim the visual dimension
 * @param value source visual coordinate
 * @param width_px line width in pixels
 * @return snapped source visual coordinate
 */
static float _axis_snap_source_panzoom_pixel_center(
    const DvzAxis* axis, const float extent[4], DvzDim dim, float value, float width_px)
{
    ANN(axis);
    ANN(extent);
    uint32_t lo_idx = dim == DVZ_DIM_X ? 0 : 2;
    uint32_t hi_idx = dim == DVZ_DIM_X ? 1 : 3;
    float displayed = _axis_forward_panzoom_coord(extent, lo_idx, hi_idx, value);
    float snapped = _axis_snap_visual_pixel_center_basis(axis, displayed, dim, width_px, true);
    return _axis_inverse_panzoom_coord(extent, lo_idx, hi_idx, snapped);
}



/**
 * Return the clamped minor tick count for one major interval.
 *
 * @param axis the axis
 * @return minor tick count per major interval
 */
static uint32_t _axis_minor_count(const DvzAxis* axis)
{
    ANN(axis);
    uint32_t count = axis->tick_policy.minor_per_interval;
    if (count > DVZ_SCENE_MAX_AXIS_MINOR_TICKS)
        count = DVZ_SCENE_MAX_AXIS_MINOR_TICKS;
    return count;
}



/**
 * Map one data coordinate to the axis' source visual coordinate.
 *
 * @param axis the axis
 * @param value data value
 * @return pre-controller visual coordinate
 */
static float _axis_data_to_axis_visual(const DvzAxis* axis, double value)
{
    return _axis_data_to_source_visual(axis, value);
}


static float _axis_plot_clip_to_fixed(float value, float plot_min, float plot_max)
{
    return plot_min + 0.5f * (value + 1.0f) * (plot_max - plot_min);
}


/**
 * Map one data coordinate to the fixed visual coordinate used by axis ticks and labels.
 *
 * @param axis the axis
 * @param value data value
 * @param extent visible source extent
 * @param plot_min fixed plot interval minimum
 * @param plot_max fixed plot interval maximum
 * @param visible_min visible data minimum
 * @param visible_max visible data maximum
 * @return fixed visual coordinate
 */
static float _axis_tick_visual_position(
    const DvzAxis* axis, double value, const float extent[4], float plot_min, float plot_max,
    double visible_min, double visible_max)
{
    ANN(axis);
    if (axis->panel != NULL && axis->panel->view2d_enabled)
    {
        uint32_t lo_idx = axis->dim == DVZ_DIM_X ? 0 : 2;
        uint32_t hi_idx = axis->dim == DVZ_DIM_X ? 1 : 3;
        float source = _axis_data_to_source_visual(axis, value);
        float plot_clip = _axis_forward_panzoom_coord(extent, lo_idx, hi_idx, source);
        return _axis_plot_clip_to_fixed(plot_clip, plot_min, plot_max);
    }
    return _axis_data_to_visual(value, visible_min, visible_max, plot_min, plot_max);
}



/**
 * Append one axis-aligned rectangle as two triangles to stack arrays.
 *
 * @param count current vertex count
 * @param positions vertex positions
 * @param colors vertex colors
 * @param x0 left
 * @param y0 bottom
 * @param x1 right
 * @param y1 top
 * @param z visual z coordinate
 * @param color rectangle color
 */
static void _axis_append_rect(
    uint32_t* count, float positions[][3], uint8_t colors[][4], float x0, float y0, float x1,
    float y1, float z, const uint8_t color[4])
{
    ANN(count);
    if (*count + 6 > 6 * DVZ_SCENE_MAX_AXIS_LINES)
        return;
    float vertices[6][3] = {
        {x0, y0, z}, {x1, y0, z}, {x1, y1, z},
        {x0, y0, z}, {x1, y1, z}, {x0, y1, z},
    };
    for (uint32_t i = 0; i < 6; i++)
    {
        uint32_t k = (*count)++;
        for (uint32_t j = 0; j < 3; j++)
            positions[k][j] = vertices[i][j];
        for (uint32_t j = 0; j < 4; j++)
            colors[k][j] = color[j];
    }
}



/**
 * Append one axis-aligned line rectangle to stack arrays.
 *
 * @param axis the axis
 * @param count current vertex count
 * @param positions vertex positions
 * @param colors vertex colors
 * @param a0 line start x
 * @param b0 line start y
 * @param a1 line end x
 * @param b1 line end y
 * @param z visual z coordinate
 * @param width_px line thickness in pixels
 * @param color line color
 * @param scale_x controller scale in X for width compensation
 * @param scale_y controller scale in Y for width compensation
 * @param plot_pixels whether the line uses the plot viewport pixel basis
 * @param snap whether to snap the line center to a pixel center
 */
static void _axis_append_line_rect(
    const DvzAxis* axis, uint32_t* count, float positions[][3], uint8_t colors[][4], float a0,
    float b0, float a1, float b1, float z, float width_px, const uint8_t color[4], float scale_x,
    float scale_y, bool plot_pixels, bool snap)
{
    ANN(axis);
    if (!(width_px > 0.0f) || !isfinite(width_px))
        return;
    if (!(scale_x > 0.0f) || !isfinite(scale_x))
        scale_x = 1.0f;
    if (!(scale_y > 0.0f) || !isfinite(scale_y))
        scale_y = 1.0f;
    float pixel_x = plot_pixels ? _axis_plot_pixel_size(axis, DVZ_DIM_X)
                                : _axis_visual_pixel_size(axis, DVZ_DIM_X);
    float pixel_y = plot_pixels ? _axis_plot_pixel_size(axis, DVZ_DIM_Y)
                                : _axis_visual_pixel_size(axis, DVZ_DIM_Y);
    float half_x = 0.5f * width_px * pixel_x / scale_x;
    float half_y = 0.5f * width_px * pixel_y / scale_y;
    if (fabsf(a1 - a0) < fabsf(b1 - b0))
    {
        float x = snap ? _axis_snap_visual_pixel_center(axis, a0, DVZ_DIM_X, width_px) : a0;
        _axis_append_rect(count, positions, colors, x - half_x, b0, x + half_x, b1, z, color);
    }
    else
    {
        float y = snap ? _axis_snap_visual_pixel_center(axis, b0, DVZ_DIM_Y, width_px) : b0;
        _axis_append_rect(count, positions, colors, a0, y - half_y, a1, y + half_y, z, color);
    }
}



/**
 * Append one tick mark as an axis-aligned rectangle.
 *
 * @param axis the axis
 * @param count current vertex count
 * @param positions vertex positions
 * @param colors vertex colors
 * @param p tick anchor in visual coordinates
 * @param x0 plot left
 * @param y0 plot bottom
 * @param z visual z coordinate
 * @param length tick length in visual units
 * @param color tick color
 * @param width tick width in pixels
 */
static void _axis_append_tick(
    const DvzAxis* axis, uint32_t* count, float positions[][3], uint8_t colors[][4], float p,
    float x0, float y0, float z, float length, const uint8_t color[4], float width)
{
    ANN(axis);
    if (!(length > 0.0f))
        return;
    if (axis->dim == DVZ_DIM_X)
        _axis_append_line_rect(
            axis, count, positions, colors, p, y0, p, y0 + length, z, width, color, 1.0f,
            1.0f, false, true);
    else
        _axis_append_line_rect(
            axis, count, positions, colors, x0, p, x0 + length, p, z, width, color, 1.0f,
            1.0f, false, true);
}


/**
 * Return whether the retained primitive visual already stores one axis geometry payload.
 *
 * @param visual retained primitive visual
 * @param vertex_count generated vertex count
 * @param positions generated vertex positions
 * @param colors generated vertex colors
 * @return whether the retained visual payload is unchanged
 */
static bool _axis_visual_cache_matches(
    DvzVisual* visual, uint32_t vertex_count, const float* positions, const uint8_t* colors)
{
    ANN(visual);
    ANN(positions);
    ANN(colors);
    if (vertex_count == 0)
        return false;

    DvzVisualDataView position_view = {0};
    DvzVisualDataView color_view = {0};
    if (dvz_visual_data(visual, "position", &position_view) != 0 ||
        dvz_visual_data(visual, "color", &color_view) != 0)
    {
        return false;
    }
    if (position_view.data == NULL || color_view.data == NULL)
        return false;
    if (
        position_view.item_count != vertex_count || color_view.item_count != vertex_count ||
        position_view.item_size != sizeof(float[3]) || color_view.item_size != sizeof(uint8_t[4]))
    {
        return false;
    }

    const size_t position_bytes = (size_t)vertex_count * sizeof(float[3]);
    const size_t color_bytes = (size_t)vertex_count * sizeof(uint8_t[4]);
    return memcmp(position_view.data, positions, position_bytes) == 0 &&
           memcmp(color_view.data, colors, color_bytes) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Rebuild the primitive visuals backing one axis.
 *
 * @param axis the axis
 */
void _axis_update_visual(DvzAxis* axis)
{
    ANN(axis);
    if (axis->visual == NULL || axis->grid_visual == NULL)
        return;

    const uint32_t max_vertices = 6 * DVZ_SCENE_MAX_AXIS_LINES;
    uint32_t fixed_vertex_count = 0;
    float (*fixed_positions)[3] = (float(*)[3])dvz_calloc(max_vertices, sizeof(float[3]));
    uint8_t (*fixed_colors)[4] = (uint8_t(*)[4])dvz_calloc(max_vertices, sizeof(uint8_t[4]));
    uint32_t grid_vertex_count = 0;
    float (*grid_positions)[3] = (float(*)[3])dvz_calloc(max_vertices, sizeof(float[3]));
    uint8_t (*grid_colors)[4] = (uint8_t(*)[4])dvz_calloc(max_vertices, sizeof(uint8_t[4]));
    if (
        fixed_positions == NULL || fixed_colors == NULL || grid_positions == NULL ||
        grid_colors == NULL)
    {
        axis->visual->visible = false;
        axis->grid_visual->visible = false;
        _axis_hide_text(axis);
        goto cleanup;
    }

    const float z = 0.0f;
    float x0 = -1.0f;
    float x1 = +1.0f;
    float y0 = -1.0f;
    float y1 = +1.0f;
    _axis_init(&axis->panel->axes[DVZ_DIM_X], axis->panel, DVZ_DIM_X);
    _axis_init(&axis->panel->axes[DVZ_DIM_Y], axis->panel, DVZ_DIM_Y);
    _axis_plot_interval(&axis->panel->axes[DVZ_DIM_X], &x0, &x1);
    _axis_plot_interval(&axis->panel->axes[DVZ_DIM_Y], &y0, &y1);

    float extent[4] = {-1.0f, +1.0f, -1.0f, +1.0f};
    if (axis->panel != NULL)
        (void)_scene_panel_panzoom_extent(axis->panel, extent);
    float source_x0 = _axis_inverse_panzoom_coord(extent, 0, 1, -1.0f);
    float source_x1 = _axis_inverse_panzoom_coord(extent, 0, 1, +1.0f);
    float source_y0 = _axis_inverse_panzoom_coord(extent, 2, 3, -1.0f);
    float source_y1 = _axis_inverse_panzoom_coord(extent, 2, 3, +1.0f);
    float scale_x = _axis_panzoom_scale(extent, DVZ_DIM_X);
    float scale_y = _axis_panzoom_scale(extent, DVZ_DIM_Y);
    float user_scale = _axis_user_scale(axis);

    double visible_min = 0.0;
    double visible_max = 0.0;
    if (!_axis_visible_domain(axis, &visible_min, &visible_max))
    {
        axis->visual->visible = false;
        axis->grid_visual->visible = false;
        _axis_hide_text(axis);
        goto cleanup;
    }
    _axis_compute_ticks(axis);

    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        float plot_min = axis->dim == DVZ_DIM_X ? x0 : y0;
        float plot_max = axis->dim == DVZ_DIM_X ? x1 : y1;
        float p = _axis_tick_visual_position(
            axis, axis->ticks[i], extent, plot_min, plot_max, visible_min, visible_max);
        if (p < plot_min - 0.0001f || p > plot_max + 0.0001f)
            continue;
        bool boundary_grid = fabsf(p - plot_min) <= 0.0001f || fabsf(p - plot_max) <= 0.0001f;
        if (axis->style.show_grid && !(axis->style.show_spine && boundary_grid))
        {
            float source_p = _axis_data_to_axis_visual(axis, axis->ticks[i]);
            float grid_width = axis->style.grid_width * user_scale;
            if (axis->dim == DVZ_DIM_X)
            {
                source_p = _axis_snap_source_panzoom_pixel_center(
                    axis, extent, DVZ_DIM_X, source_p, grid_width);
                _axis_append_line_rect(
                    axis, &grid_vertex_count, grid_positions, grid_colors, source_p, source_y0,
                    source_p, source_y1, z, grid_width, axis->style.grid_color, scale_x, scale_y,
                    true, false);
            }
            else
            {
                source_p = _axis_snap_source_panzoom_pixel_center(
                    axis, extent, DVZ_DIM_Y, source_p, grid_width);
                _axis_append_line_rect(
                    axis, &grid_vertex_count, grid_positions, grid_colors, source_x0, source_p,
                    source_x1, source_p, z, grid_width, axis->style.grid_color, scale_x, scale_y,
                    true, false);
            }
        }
    }

    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        float plot_min = axis->dim == DVZ_DIM_X ? x0 : y0;
        float plot_max = axis->dim == DVZ_DIM_X ? x1 : y1;
        float p = _axis_tick_visual_position(
            axis, axis->ticks[i], extent, plot_min, plot_max, visible_min, visible_max);
        if (p < plot_min - 0.0001f || p > plot_max + 0.0001f)
            continue;
        if (axis->style.show_major_ticks)
        {
            float len = _axis_tick_length(axis, axis->style.major_tick_length * user_scale);
            _axis_append_tick(
                axis, &fixed_vertex_count, fixed_positions, fixed_colors, p, x0, y0, z, len,
                axis->style.major_tick_color, axis->style.major_tick_width * user_scale);
        }
        if (axis->style.show_minor_ticks && i + 1 < axis->tick_count)
        {
            uint32_t minor_count = _axis_minor_count(axis);
            float len = _axis_tick_length(axis, axis->style.minor_tick_length * user_scale);
            double delta = (axis->ticks[i + 1] - axis->ticks[i]) / (double)(minor_count + 1);
            for (uint32_t j = 1; j <= minor_count; j++)
            {
                double value = axis->ticks[i] + (double)j * delta;
                float mp = _axis_tick_visual_position(
                    axis, value, extent, plot_min, plot_max, visible_min, visible_max);
                if (mp < plot_min - 0.0001f || mp > plot_max + 0.0001f)
                    continue;
                _axis_append_tick(
                    axis, &fixed_vertex_count, fixed_positions, fixed_colors, mp, x0, y0, z, len,
                    axis->style.minor_tick_color, axis->style.minor_tick_width * user_scale);
            }
        }
    }

    if (axis->style.show_spine)
    {
        if (axis->dim == DVZ_DIM_X)
        {
            float y = y0 + 0.5f * axis->style.spine_width *
                               user_scale * _axis_visual_pixel_size(axis, DVZ_DIM_Y);
            _axis_append_line_rect(
                axis, &fixed_vertex_count, fixed_positions, fixed_colors, x0, y, x1, y, z,
                axis->style.spine_width * user_scale, axis->style.spine_color, 1.0f, 1.0f, false,
                true);
        }
        else
        {
            float x = x0 + 0.5f * axis->style.spine_width *
                               user_scale * _axis_visual_pixel_size(axis, DVZ_DIM_X);
            _axis_append_line_rect(
                axis, &fixed_vertex_count, fixed_positions, fixed_colors, x, y0, x, y1, z,
                axis->style.spine_width * user_scale, axis->style.spine_color, 1.0f, 1.0f, false,
                true);
        }
    }

    axis->visual->visible = axis->enabled && fixed_vertex_count > 0;
    axis->grid_visual->visible = axis->enabled && grid_vertex_count > 0;
    if (fixed_vertex_count > 0)
    {
        DvzVisualDataUpdate updates[] = {
            {.attr_name = "position", .data = fixed_positions, .item_count = fixed_vertex_count},
            {.attr_name = "color", .data = fixed_colors, .item_count = fixed_vertex_count},
        };
        if (!_axis_visual_cache_matches(
                axis->visual, fixed_vertex_count, &fixed_positions[0][0], &fixed_colors[0][0]))
        {
            (void)dvz_visual_set_data_many(axis->visual, updates, 2);
        }
    }
    if (grid_vertex_count > 0)
    {
        DvzVisualDataUpdate updates[] = {
            {.attr_name = "position", .data = grid_positions, .item_count = grid_vertex_count},
            {.attr_name = "color", .data = grid_colors, .item_count = grid_vertex_count},
        };
        if (!_axis_visual_cache_matches(
                axis->grid_visual, grid_vertex_count, &grid_positions[0][0], &grid_colors[0][0]))
        {
            (void)dvz_visual_set_data_many(axis->grid_visual, updates, 2);
        }
    }
    if (fixed_vertex_count == 0 && grid_vertex_count == 0)
        _axis_hide_text(axis);
    else
        _axis_update_text(axis, x0, x1, y0, y1, visible_min, visible_max);
    axis->dirty = false;

cleanup:
    dvz_free(fixed_positions);
    dvz_free(fixed_colors);
    dvz_free(grid_positions);
    dvz_free(grid_colors);
}



/**
 * Rebuild all enabled panel axis visuals before FramePlan emission.
 *
 * @param figure the figure
 */
void _scene_prepare_axis_visuals(DvzFigure* figure)
{
    if (figure == NULL)
        return;
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        _scene_panel_refresh_axis_reserve(panel);
        for (uint32_t pass = 0; pass < 2; pass++)
        {
            for (uint32_t dim = 0; dim < 2; dim++)
            {
                DvzAxis* axis = &panel->axes[dim];
                if (axis->panel == NULL || axis->visual == NULL || axis->grid_visual == NULL)
                    continue;
                _axis_update_visual(axis);
            }
            if (pass == 0)
                _scene_panel_refresh_axis_reserve(panel);
        }
    }
}
