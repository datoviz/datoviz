/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene 2D axes                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"
#include "_scene_emit.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define AXIS_EPS 1e-12



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the default WIP axis tick policy.
 *
 * @return default axis tick policy
 */
static DvzAxisTickPolicy _axis_default_tick_policy(void)
{
    return (DvzAxisTickPolicy){.target_count = 6, .min_pixel_spacing = 50.0f};
}



/**
 * Return the default WIP axis line style.
 *
 * @return default axis style
 */
static DvzAxisStyle _axis_default_style(void)
{
    return (DvzAxisStyle){
        .spine_width = 1.0f,
        .major_tick_width = 1.0f,
        .grid_width = 1.0f,
        .major_tick_length = 0.035f,
        .plot_margin_left = 0.10f,
        .plot_margin_right = 0.04f,
        .plot_margin_bottom = 0.10f,
        .plot_margin_top = 0.04f,
        .spine_color = {220, 220, 220, 255},
        .major_tick_color = {220, 220, 220, 255},
        .grid_color = {90, 95, 105, 180},
        .show_spine = true,
        .show_major_ticks = true,
        .show_grid = false,
    };
}



/**
 * Return whether a dimension is supported by the first WIP 2D axis slice.
 *
 * @param dim the dimension
 * @return true for X/Y
 */
static bool _axis_dim_supported(DvzDim dim)
{
    return dim == DVZ_DIM_X || dim == DVZ_DIM_Y;
}



/**
 * Return the panel-owned axis slot for a supported dimension.
 *
 * @param panel the panel
 * @param dim the dimension
 * @return the axis slot, or NULL
 */
static DvzAxis* _panel_axis_slot(DvzPanel* panel, DvzDim dim)
{
    if (panel == NULL || !_axis_dim_supported(dim))
        return NULL;
    return &panel->axes[(uint32_t)dim];
}



/**
 * Initialize one panel-owned axis slot if it has not been initialized yet.
 *
 * @param axis the axis
 * @param panel the owning panel
 * @param dim the dimension
 */
static void _axis_init(DvzAxis* axis, DvzPanel* panel, DvzDim dim)
{
    ANN(axis);
    ANN(panel);
    if (axis->panel != NULL)
        return;
    axis->panel = panel;
    axis->dim = dim;
    axis->enabled = false;
    axis->dirty = true;
    axis->domain = (DvzDataDomain){.min = -1.0, .max = +1.0};
    axis->tick_policy = _axis_default_tick_policy();
    axis->style = _axis_default_style();
}


/**
 * Return the visual-space plot interval for one axis dimension.
 *
 * @param axis the axis
 * @param out_min output plot minimum
 * @param out_max output plot maximum
 */
static void _axis_plot_interval(const DvzAxis* axis, float* out_min, float* out_max)
{
    ANN(axis);
    ANN(out_min);
    ANN(out_max);
    const DvzAxisStyle* style = &axis->style;
    if (axis->dim == DVZ_DIM_X)
    {
        *out_min = -1.0f + style->plot_margin_left;
        *out_max = +1.0f - style->plot_margin_right;
    }
    else
    {
        *out_min = -1.0f + style->plot_margin_bottom;
        *out_max = +1.0f - style->plot_margin_top;
    }
    if (*out_max <= *out_min)
    {
        *out_min = -1.0f;
        *out_max = +1.0f;
    }
}



/**
 * Return a nice step size with the v0.3 1/2/5 ladder.
 *
 * @param range raw step size
 * @param round whether to round to the nearest nice value
 * @return nice step size
 */
static double _axis_nice_number(double range, bool round)
{
    if (!(range > 0.0) || !isfinite(range))
        return 1.0;
    double exponent = floor(log10(range));
    double fraction = range / pow(10.0, exponent);
    double nice_fraction = 10.0;
    if (round)
    {
        if (fraction < 1.5)
            nice_fraction = 1.0;
        else if (fraction < 3.0)
            nice_fraction = 2.0;
        else if (fraction < 7.0)
            nice_fraction = 5.0;
    }
    else
    {
        if (fraction <= 1.0)
            nice_fraction = 1.0;
        else if (fraction <= 2.0)
            nice_fraction = 2.0;
        else if (fraction <= 5.0)
            nice_fraction = 5.0;
    }
    return nice_fraction * pow(10.0, exponent);
}



/**
 * Map one data coordinate to fixed visual coordinates for an interval.
 *
 * @param value data value
 * @param min interval minimum
 * @param max interval maximum
 * @return visual coordinate
 */
static float _axis_data_to_visual(
    double value, double min, double max, float visual_min, float visual_max)
{
    double denom = max - min;
    if (fabs(denom) < AXIS_EPS)
        return 0.0f;
    double t = (value - min) / denom;
    return (float)((double)visual_min + ((double)visual_max - (double)visual_min) * t);
}



/**
 * Map one visual coordinate to data coordinates for an axis.
 *
 * @param axis the axis
 * @param value visual coordinate
 * @return data coordinate
 */
static double _axis_visual_to_data(const DvzAxis* axis, float value)
{
    ANN(axis);
    float visual_min = -1.0f;
    float visual_max = +1.0f;
    _axis_plot_interval(axis, &visual_min, &visual_max);
    double denom = (double)visual_max - (double)visual_min;
    if (fabs(denom) < AXIS_EPS)
        return axis->domain.min;
    double t = ((double)value - (double)visual_min) / denom;
    return axis->domain.min + t * (axis->domain.max - axis->domain.min);
}



/**
 * Return the visible data interval for an axis.
 *
 * @param axis the axis
 * @param out_min output data minimum
 * @param out_max output data maximum
 * @return whether the interval was written
 */
static bool _axis_visible_domain(const DvzAxis* axis, double* out_min, double* out_max)
{
    ANN(axis);
    ANN(out_min);
    ANN(out_max);
    float extent[4] = {-1.0f, +1.0f, -1.0f, +1.0f};
    if (axis->panel != NULL && axis->panel->panzoom != NULL)
        (void)dvz_panzoom_extent(axis->panel->panzoom, extent);
    uint32_t lo_idx = axis->dim == DVZ_DIM_X ? 0 : 2;
    uint32_t hi_idx = axis->dim == DVZ_DIM_X ? 1 : 3;
    double a = _axis_visual_to_data(axis, extent[lo_idx]);
    double b = _axis_visual_to_data(axis, extent[hi_idx]);
    *out_min = fmin(a, b);
    *out_max = fmax(a, b);
    return isfinite(*out_min) && isfinite(*out_max) && *out_max > *out_min;
}



/**
 * Compute major tick values for one axis.
 *
 * @param axis the axis
 */
static void _axis_compute_ticks(DvzAxis* axis)
{
    ANN(axis);
    axis->tick_count = 0;
    double min = 0.0;
    double max = 0.0;
    if (!_axis_visible_domain(axis, &min, &max))
        return;

    uint32_t target = axis->tick_policy.target_count;
    if (axis->panel != NULL && axis->tick_policy.min_pixel_spacing > 0.0f)
    {
        float panel_x = 0.0f;
        float panel_y = 0.0f;
        float panel_width = 0.0f;
        float panel_height = 0.0f;
        _scene_panel_pixel_rect(
            axis->panel, &panel_x, &panel_y, &panel_width, &panel_height);
        float pixels = axis->dim == DVZ_DIM_X ? panel_width : panel_height;
        uint32_t pixel_target = (uint32_t)floorf(pixels / axis->tick_policy.min_pixel_spacing) + 1;
        if (pixel_target > 0 && pixel_target < target)
            target = pixel_target;
    }
    if (target < 2)
        target = 2;
    if (target > DVZ_SCENE_MAX_AXIS_TICKS)
        target = DVZ_SCENE_MAX_AXIS_TICKS;

    double range = max - min;
    if (!isfinite(range) || range <= AXIS_EPS)
        return;
    double raw_step = range / (double)target;
    double step = _axis_nice_number(raw_step, true);
    if (axis->tick_lstep > 0.0 && isfinite(axis->tick_lstep))
    {
        double current_density = range / axis->tick_lstep;
        if (current_density >= 0.5 * (double)target && current_density <= 2.0 * (double)target)
            step = axis->tick_lstep;
    }
    if (!isfinite(step) || step <= AXIS_EPS)
        return;

    double lmin = floor(min / step) * step;
    double lmax = ceil(max / step) * step;
    if (!isfinite(lmin) || !isfinite(lmax) || !(lmax >= lmin))
        return;
    axis->tick_lmin = lmin;
    axis->tick_lmax = lmax;
    axis->tick_lstep = step;
    for (double value = lmin; value <= lmax + 0.5 * step; value += step)
    {
        if (axis->tick_count >= DVZ_SCENE_MAX_AXIS_TICKS)
            break;
        axis->ticks[axis->tick_count++] = value;
    }
}



/**
 * Append one segment line to stack arrays.
 *
 * @param count current line count
 * @param starts start positions
 * @param ends end positions
 * @param colors line colors
 * @param widths line widths
 * @param start line start
 * @param end line end
 * @param color line color
 * @param width line width
 */
static void _axis_append_line(
    uint32_t* count, float starts[][3], float ends[][3], uint8_t colors[][4], float widths[],
    const float start[3], const float end[3], const uint8_t color[4], float width)
{
    ANN(count);
    if (*count >= DVZ_SCENE_MAX_AXIS_LINES)
        return;
    uint32_t i = (*count)++;
    for (uint32_t j = 0; j < 3; j++)
    {
        starts[i][j] = start[j];
        ends[i][j] = end[j];
    }
    for (uint32_t j = 0; j < 4; j++)
        colors[i][j] = color[j];
    widths[i] = width;
}



/**
 * Rebuild the fixed-space segment visual backing one axis.
 *
 * @param axis the axis
 */
static void _axis_update_visual(DvzAxis* axis)
{
    ANN(axis);
    if (axis->visual == NULL)
        return;

    uint32_t count = 0;
    float starts[DVZ_SCENE_MAX_AXIS_LINES][3] = {{0}};
    float ends[DVZ_SCENE_MAX_AXIS_LINES][3] = {{0}};
    uint8_t colors[DVZ_SCENE_MAX_AXIS_LINES][4] = {{0}};
    float widths[DVZ_SCENE_MAX_AXIS_LINES] = {0};
    const float z = 0.0f;
    float x0 = -1.0f;
    float x1 = +1.0f;
    float y0 = -1.0f;
    float y1 = +1.0f;
    _axis_plot_interval(&axis->panel->axes[DVZ_DIM_X], &x0, &x1);
    _axis_plot_interval(&axis->panel->axes[DVZ_DIM_Y], &y0, &y1);

    if (axis->style.show_spine)
    {
        if (axis->dim == DVZ_DIM_X)
            _axis_append_line(
                &count, starts, ends, colors, widths, (float[3]){x0, y0, z},
                (float[3]){x1, y0, z}, axis->style.spine_color,
                axis->style.spine_width);
        else
            _axis_append_line(
                &count, starts, ends, colors, widths, (float[3]){x0, y0, z},
                (float[3]){x0, y1, z}, axis->style.spine_color,
                axis->style.spine_width);
    }

    double visible_min = 0.0;
    double visible_max = 0.0;
    if (!_axis_visible_domain(axis, &visible_min, &visible_max))
        return;
    _axis_compute_ticks(axis);
    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        float plot_min = axis->dim == DVZ_DIM_X ? x0 : y0;
        float plot_max = axis->dim == DVZ_DIM_X ? x1 : y1;
        float p =
            _axis_data_to_visual(axis->ticks[i], visible_min, visible_max, plot_min, plot_max);
        if (p < plot_min - 0.0001f || p > plot_max + 0.0001f)
            continue;
        if (axis->style.show_grid)
        {
            if (axis->dim == DVZ_DIM_X)
                _axis_append_line(
                    &count, starts, ends, colors, widths, (float[3]){p, y0, z},
                    (float[3]){p, y1, z}, axis->style.grid_color,
                    axis->style.grid_width);
            else
                _axis_append_line(
                    &count, starts, ends, colors, widths, (float[3]){x0, p, z},
                    (float[3]){x1, p, z}, axis->style.grid_color,
                    axis->style.grid_width);
        }
        if (axis->style.show_major_ticks)
        {
            float len = axis->style.major_tick_length;
            if (axis->dim == DVZ_DIM_X)
                _axis_append_line(
                    &count, starts, ends, colors, widths, (float[3]){p, y0, z},
                    (float[3]){p, y0 - len, z}, axis->style.major_tick_color,
                    axis->style.major_tick_width);
            else
                _axis_append_line(
                    &count, starts, ends, colors, widths, (float[3]){x0, p, z},
                    (float[3]){x0 - len, p, z}, axis->style.major_tick_color,
                    axis->style.major_tick_width);
        }
    }

    axis->visual->visible = axis->enabled && count > 0;
    if (count == 0)
        return;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = count},
        {.attr_name = "position_end", .data = ends, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "stroke_width", .data = widths, .item_count = count},
    };
    (void)dvz_visual_set_data_many(axis->visual, updates, 4);
    axis->dirty = false;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Set a panel data domain for one axis dimension.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @param min data minimum
 * @param max data maximum
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_domain(DvzPanel* panel, DvzDim dim, double min, double max)
{
    DvzAxis* axis = _panel_axis_slot(panel, dim);
    if (axis == NULL || !isfinite(min) || !isfinite(max) || !(max > min))
        return -1;
    _axis_init(axis, panel, dim);
    axis->domain = (DvzDataDomain){.min = min, .max = max};
    axis->tick_lstep = 0.0;
    axis->dirty = true;
    axis->version++;
    return 0;
}


/**
 * Normalize tightly packed 3D data positions through the panel X/Y domains.
 *
 * @param panel the panel
 * @param data_positions tightly packed input positions, 3 floats per item
 * @param visual_positions tightly packed output positions, 3 floats per item
 * @param count number of positions
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_data_to_visual_positions(
    DvzPanel* panel, const float* data_positions, float* visual_positions, uint32_t count)
{
    if (panel == NULL || data_positions == NULL || visual_positions == NULL)
        return -1;
    DvzAxis* x_axis = _panel_axis_slot(panel, DVZ_DIM_X);
    DvzAxis* y_axis = _panel_axis_slot(panel, DVZ_DIM_Y);
    bool has_x = x_axis != NULL && x_axis->panel != NULL;
    bool has_y = y_axis != NULL && y_axis->panel != NULL;
    if (has_x && fabs(x_axis->domain.max - x_axis->domain.min) < AXIS_EPS)
        return -1;
    if (has_y && fabs(y_axis->domain.max - y_axis->domain.min) < AXIS_EPS)
        return -1;

    for (uint32_t i = 0; i < count; i++)
    {
        float x = data_positions[3 * i + 0];
        float y = data_positions[3 * i + 1];
        float z = data_positions[3 * i + 2];
        if (!isfinite(x) || !isfinite(y) || !isfinite(z))
            return -1;
        if (has_x)
        {
            float x0 = -1.0f;
            float x1 = +1.0f;
            _axis_plot_interval(x_axis, &x0, &x1);
            visual_positions[3 * i + 0] =
                _axis_data_to_visual((double)x, x_axis->domain.min, x_axis->domain.max, x0, x1);
        }
        else
        {
            visual_positions[3 * i + 0] = x;
        }
        if (has_y)
        {
            float y0 = -1.0f;
            float y1 = +1.0f;
            _axis_plot_interval(y_axis, &y0, &y1);
            visual_positions[3 * i + 1] =
                _axis_data_to_visual((double)y, y_axis->domain.min, y_axis->domain.max, y0, y1);
        }
        else
        {
            visual_positions[3 * i + 1] = y;
        }
        visual_positions[3 * i + 2] = z;
    }
    return 0;
}



/**
 * Return a panel-owned axis, creating its fixed segment visual on first use.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @return the panel-owned axis, or NULL
 */
DvzAxis* dvz_panel_axis(DvzPanel* panel, DvzDim dim)
{
    DvzAxis* axis = _panel_axis_slot(panel, dim);
    if (axis == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    _axis_init(axis, panel, dim);
    if (axis->visual == NULL)
    {
        axis->visual = dvz_segment(panel->figure->scene, 0);
        if (axis->visual == NULL)
            return NULL;
        axis->visual->visible = false;
        DvzVisualAttachDesc attach = {.z_layer = 1000, .controller_mode = DVZ_CONTROLLER_FIXED};
        if (dvz_panel_add_visual(panel, axis->visual, &attach) != 0)
        {
            axis->visual = NULL;
            return NULL;
        }
    }
    axis->enabled = true;
    axis->dirty = true;
    return axis;
}



/**
 * Show or hide one panel-owned axis.
 *
 * @param axis the axis
 * @param visible whether the axis is visible
 * @return whether the axis was updated
 */
bool dvz_axis_set_visible(DvzAxis* axis, bool visible)
{
    if (axis == NULL)
        return false;
    axis->enabled = visible;
    axis->dirty = true;
    axis->version++;
    if (axis->visual != NULL && !visible)
        axis->visual->visible = false;
    return true;
}



/**
 * Enable or disable grid lines for one panel-owned axis.
 *
 * @param axis the axis
 * @param visible whether grid lines are visible
 * @return whether the axis was updated
 */
bool dvz_axis_set_grid(DvzAxis* axis, bool visible)
{
    if (axis == NULL)
        return false;
    axis->style.show_grid = visible;
    axis->dirty = true;
    axis->version++;
    return true;
}



/**
 * Set the label stored on one panel-owned axis.
 *
 * @param axis the axis
 * @param label label string, or NULL to clear
 * @return whether the axis was updated
 */
bool dvz_axis_set_label(DvzAxis* axis, const char* label)
{
    if (axis == NULL)
        return false;
    dvz_strlcpy(axis->label, label != NULL ? label : "", sizeof(axis->label));
    axis->version++;
    return true;
}



/**
 * Set the tick policy for one panel-owned axis.
 *
 * @param axis the axis
 * @param policy tick policy, or NULL for defaults
 * @return whether the axis was updated
 */
bool dvz_axis_set_tick_policy(DvzAxis* axis, const DvzAxisTickPolicy* policy)
{
    if (axis == NULL)
        return false;
    axis->tick_policy = policy != NULL ? *policy : _axis_default_tick_policy();
    axis->tick_lstep = 0.0;
    axis->dirty = true;
    axis->version++;
    return true;
}



/**
 * Set the line style for one panel-owned axis.
 *
 * @param axis the axis
 * @param style axis style, or NULL for defaults
 * @return whether the axis was updated
 */
bool dvz_axis_set_style(DvzAxis* axis, const DvzAxisStyle* style)
{
    if (axis == NULL)
        return false;
    axis->style = style != NULL ? *style : _axis_default_style();
    axis->dirty = true;
    axis->version++;
    return true;
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
        for (uint32_t dim = 0; dim < 2; dim++)
        {
            DvzAxis* axis = &panel->axes[dim];
            if (axis->panel == NULL || axis->visual == NULL)
                continue;
            _axis_update_visual(axis);
        }
    }
}
