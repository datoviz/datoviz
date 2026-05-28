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
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"
#include "scene_emit/scene_emit.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define AXIS_EPS 1e-12
#define AXIS_TEXT_TICK_SIZE 11.0f
#define AXIS_TEXT_LABEL_SIZE 13.0f
#define AXIS_TEXT_TICK_GAP 6.0f
#define AXIS_TEXT_LABEL_GAP 28.0f
#define AXIS_TEXT_Y_LABEL_ANGLE -1.57079632679f



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
    return (DvzAxisTickPolicy){
        .target_count = 6, .min_pixel_spacing = 100.0f, .minor_per_interval = 4};
}



/**
 * Return the default WIP axis line and text style.
 *
 * @return default axis style
 */
static DvzAxisStyle _axis_default_style(void)
{
    return (DvzAxisStyle){
        .spine_width = 1.0f,
        .major_tick_width = 1.0f,
        .minor_tick_width = 1.0f,
        .grid_width = 1.0f,
        .major_tick_length = 9.0f,
        .minor_tick_length = 5.0f,
        .reserve_px = 0.0f,
        .tick_gap_px = AXIS_TEXT_TICK_GAP,
        .label_gap_px = AXIS_TEXT_LABEL_GAP,
        .tick_size_px = AXIS_TEXT_TICK_SIZE,
        .label_size_px = AXIS_TEXT_LABEL_SIZE,
        .text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        .plot_margin_left = 0.0f,
        .plot_margin_right = 0.0f,
        .plot_margin_bottom = 0.0f,
        .plot_margin_top = 0.0f,
        .spine_color = {220, 220, 220, 255},
        .major_tick_color = {220, 220, 220, 255},
        .minor_tick_color = {170, 170, 170, 220},
        .grid_color = {90, 95, 105, 180},
        .show_spine = true,
        .show_major_ticks = true,
        .show_minor_ticks = true,
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
 * Apply the axis text renderer to the derived text visual.
 *
 * @param axis the axis
 * @return whether the renderer was applied
 */
static bool _axis_apply_text_renderer(DvzAxis* axis)
{
    ANN(axis);
    if (axis->text_visual == NULL)
        return true;
    return _scene_adornment_text_visual_set_renderer(
               axis->text_visual, axis->style.text_renderer) == 0;
}


/**
 * Return one axis reserve contribution in logical pixels.
 *
 * @param axis the axis
 * @return reserve contribution in pixels
 */
static float _axis_reserve_px(const DvzAxis* axis)
{
    ANN(axis);
    if (!axis->enabled)
        return 0.0f;
    if (axis->style.reserve_px > 0.0f && isfinite(axis->style.reserve_px))
        return axis->style.reserve_px;
    return 0.0f;
}


/**
 * Mark one axis layout and derived visuals dirty.
 *
 * @param axis the axis
 */
static void _axis_mark_dirty(DvzAxis* axis)
{
    if (axis == NULL)
        return;
    axis->tick_cache_valid = false;
    axis->dirty = true;
    axis->version++;
    _scene_panel_refresh_axis_reserve(axis->panel);
    _scene_notify_request_frame(axis->panel != NULL ? axis->panel->figure : NULL);
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
 * Refresh aggregate attached axis reserve for one panel.
 *
 * @param panel the panel
 */
void _scene_panel_refresh_axis_reserve(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    DvzPanelReserve reserve = {0};
    DvzAxis* x_axis = &panel->axes[DVZ_DIM_X];
    DvzAxis* y_axis = &panel->axes[DVZ_DIM_Y];
    if (x_axis->panel == panel)
        reserve.bottom_px = _axis_reserve_px(x_axis);
    if (y_axis->panel == panel)
        reserve.left_px = _axis_reserve_px(y_axis);
    _scene_panel_set_axis_reserve(panel, &reserve);
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
    axis->domain_set = false;
    axis->domain = (DvzDataDomain){.min = -1.0, .max = +1.0};
    axis->tick_policy = _axis_default_tick_policy();
    axis->style = _axis_default_style();
    axis->tick_cache_valid = false;
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
    float plot[4] = {-1.0f, +1.0f, -1.0f, +1.0f};
    if (axis->panel != NULL)
        _scene_panel_plot_visual_rect(axis->panel, plot);
    if (axis->dim == DVZ_DIM_X)
    {
        *out_min = plot[0] + style->plot_margin_left;
        *out_max = plot[1] - style->plot_margin_right;
    }
    else
    {
        *out_min = plot[2] + style->plot_margin_bottom;
        *out_max = plot[3] - style->plot_margin_top;
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
 * Return the approximate pixel span available to one axis.
 *
 * @param axis the axis
 * @return pixel span
 */
static float _axis_pixel_span(const DvzAxis* axis)
{
    ANN(axis);
    if (axis->panel == NULL)
        return 0.0f;
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(axis->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    return axis->dim == DVZ_DIM_X ? panel_width : panel_height;
}



/**
 * Return the visible-domain target tick count for one axis.
 *
 * @param axis the axis
 * @param pixel_span output pixel span used for the target
 * @return target visible tick count
 */
static uint32_t _axis_target_tick_count(const DvzAxis* axis, float* pixel_span)
{
    ANN(axis);
    ANN(pixel_span);
    *pixel_span = _axis_pixel_span(axis);
    uint32_t target = axis->tick_policy.target_count;
    if (*pixel_span > 0.0f && axis->tick_policy.min_pixel_spacing > 0.0f)
    {
        uint32_t pixel_target =
            (uint32_t)floorf(*pixel_span / axis->tick_policy.min_pixel_spacing) + 1;
        if (pixel_target > target)
            target = pixel_target;
    }
    if (target < 2)
        target = 2;
    if (target > DVZ_SCENE_MAX_AXIS_TICKS)
        target = DVZ_SCENE_MAX_AXIS_TICKS;
    return target;
}



/**
 * Return a major tick length in visual units, with the style length interpreted as pixels.
 *
 * @param axis the axis
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
 * Snap one visual coordinate to a pixel center.
 *
 * @param axis the axis
 * @param value visual coordinate
 * @param dim the visual dimension
 * @return snapped visual coordinate
 */
static float _axis_snap_visual_pixel_center(const DvzAxis* axis, float value, DvzDim dim)
{
    ANN(axis);
    if (axis->panel == NULL)
        return value;
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(axis->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    float span = dim == DVZ_DIM_X ? panel_width : panel_height;
    if (!(span > 0.0f) || !isfinite(span))
        return value;
    float pixel = (value + 1.0f) * 0.5f * span;
    return 2.0f * (floorf(pixel) + 0.5f) / span - 1.0f;
}



/**
 * Convert one fixed visual-space coordinate to panel-local pixel coordinates.
 *
 * @param axis the axis owning the panel
 * @param visual_x visual-space x coordinate
 * @param visual_y visual-space y coordinate
 * @param out_x output x coordinate in panel-local pixels
 * @param out_y output y coordinate in panel-local pixels from the panel top
 */
static void _axis_visual_to_pixels(
    const DvzAxis* axis, float visual_x, float visual_y, float* out_x, float* out_y)
{
    ANN(axis);
    ANN(out_x);
    ANN(out_y);
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(axis->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    (void)panel_x;
    (void)panel_y;
    *out_x = 0.5f * (visual_x + 1.0f) * panel_width;
    *out_y = 0.5f * (1.0f - visual_y) * panel_height;
}



/**
 * Format one numeric tick value for the first rendered 2D axis label slice.
 *
 * @param value the tick value
 * @param step the major tick step
 * @param out output string buffer
 * @param out_size output string buffer size
 */
static void _axis_format_tick(double value, double step, char* out, uint32_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return;
    if (isfinite(step) && step > 0.0 && fabs(value) < 0.5 * step * 1e-9)
        value = 0.0;
    dvz_snprintf(out, out_size, "%.6g", value);
}



/**
 * Return whether two axis text layouts are byte-identical.
 *
 * @param axis the axis
 * @param count text item count
 * @param labels text labels
 * @param positions text positions
 * @param anchors text anchors
 * @param sizes text sizes
 * @param colors text colors
 * @param angles text angles
 * @return whether the cached layout matches
 */
static bool _axis_text_cache_matches(
    const DvzAxis* axis, uint32_t count, char labels[][DVZ_SCENE_LABEL_SIZE],
    float positions[][3], float anchors[][2], float* sizes, uint8_t colors[][4],
    float* angles)
{
    ANN(axis);
    if (axis->text_count != count)
        return false;
    for (uint32_t i = 0; i < count; i++)
    {
        if (strcmp(axis->text_labels[i], labels[i]) != 0)
            return false;
        for (uint32_t j = 0; j < 3; j++)
        {
            if (fabsf(axis->text_positions[i][j] - positions[i][j]) > 1e-5f)
                return false;
        }
        for (uint32_t j = 0; j < 2; j++)
        {
            if (fabsf(axis->text_anchors[i][j] - anchors[i][j]) > 1e-5f)
                return false;
        }
        if (fabsf(axis->text_sizes[i] - sizes[i]) > 1e-5f)
            return false;
        if (memcmp(axis->text_colors[i], colors[i], 4) != 0)
            return false;
        if (fabsf(axis->text_angles[i] - angles[i]) > 1e-5f)
            return false;
    }
    return true;
}



/**
 * Store a successfully emitted axis text layout in the axis cache.
 *
 * @param axis the axis
 * @param count text item count
 * @param labels text labels
 * @param positions text positions
 * @param anchors text anchors
 * @param sizes text sizes
 * @param colors text colors
 * @param angles text angles
 */
static void _axis_text_cache_store(
    DvzAxis* axis, uint32_t count, char labels[][DVZ_SCENE_LABEL_SIZE],
    float positions[][3], float anchors[][2], float* sizes, uint8_t colors[][4],
    float* angles)
{
    ANN(axis);
    axis->text_count = count;
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_strlcpy(axis->text_labels[i], labels[i], sizeof(axis->text_labels[i]));
        for (uint32_t j = 0; j < 3; j++)
            axis->text_positions[i][j] = positions[i][j];
        for (uint32_t j = 0; j < 2; j++)
            axis->text_anchors[i][j] = anchors[i][j];
        axis->text_sizes[i] = sizes[i];
        for (uint32_t j = 0; j < 4; j++)
            axis->text_colors[i][j] = colors[i][j];
        axis->text_angles[i] = angles[i];
    }
}



/**
 * Hide the derived text visual for an axis.
 *
 * @param axis the axis
 */
static void _axis_hide_text(DvzAxis* axis)
{
    ANN(axis);
    axis->text_count = 0;
    if (axis->text_visual != NULL)
    {
        if (axis->text_visual->visible)
            dvz_visual_set_visible(axis->text_visual, false);
        if (axis->text_visual->text.glyph_visual != NULL &&
            axis->text_visual->text.glyph_visual->visible)
            dvz_visual_set_visible(axis->text_visual->text.glyph_visual, false);
    }
}



/**
 * Ensure one derived screen-space text visual exists for an axis.
 *
 * @param axis the axis
 * @return whether the text visual exists
 */
static bool _axis_ensure_text_visual(DvzAxis* axis)
{
    ANN(axis);
    if (axis->text_visual != NULL)
        return true;
    if (axis->panel == NULL || axis->panel->figure == NULL || axis->panel->figure->scene == NULL)
        return false;
    axis->text_visual =
        _scene_adornment_text_visual(axis->panel->figure->scene, axis->style.text_renderer);
    if (axis->text_visual == NULL)
        return false;
    axis->text_visual->visible = false;
    DvzVisualAttachDesc attach = {.z_layer = 1001, .controller_mode = DVZ_CONTROLLER_FIXED};
    if (dvz_panel_add_visual(axis->panel, axis->text_visual, &attach) != 0)
    {
        axis->text_visual = NULL;
        return false;
    }
    return true;
}



/**
 * Append one text item to axis text layout arrays.
 *
 * @param count current text item count
 * @param labels text labels
 * @param strings string pointer table
 * @param positions text positions
 * @param anchors text anchors
 * @param sizes text sizes
 * @param colors text colors
 * @param angles text angles
 * @param label item label
 * @param x item x coordinate in pixels
 * @param y item y coordinate in pixels
 * @param anchor_x text anchor x
 * @param anchor_y text anchor y
 * @param size text size in pixels
 * @param color text color
 * @param angle text angle in radians
 */
static void _axis_append_text_item(
    uint32_t* count, char labels[][DVZ_SCENE_LABEL_SIZE], const char** strings,
    float positions[][3], float anchors[][2], float* sizes, uint8_t colors[][4], float* angles,
    const char* label, float x, float y, float anchor_x, float anchor_y, float size,
    const uint8_t color[4], float angle)
{
    ANN(count);
    ANN(labels);
    ANN(strings);
    ANN(positions);
    ANN(anchors);
    ANN(sizes);
    ANN(colors);
    ANN(angles);
    ANN(label);
    ANN(color);
    if (*count >= DVZ_SCENE_MAX_AXIS_TICKS + 1)
        return;
    uint32_t i = (*count)++;
    dvz_strlcpy(labels[i], label, DVZ_SCENE_LABEL_SIZE);
    strings[i] = labels[i];
    positions[i][0] = x;
    positions[i][1] = y;
    positions[i][2] = 0.0f;
    anchors[i][0] = anchor_x;
    anchors[i][1] = anchor_y;
    sizes[i] = size;
    for (uint32_t j = 0; j < 4; j++)
        colors[i][j] = color[j];
    angles[i] = angle;
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
 * Return a positive axis text size, falling back to the built-in default.
 *
 * @param value configured size in logical pixels
 * @param fallback fallback size in logical pixels
 * @return resolved size in logical pixels
 */
static float _axis_text_size(float value, float fallback)
{
    return value > 0.0f && isfinite(value) ? value : fallback;
}


/**
 * Fill the visible tick values for the current tick step.
 *
 * @param axis the axis
 * @param min visible domain minimum
 * @param max visible domain maximum
 * @param step major tick step
 */
static void _axis_fill_visible_ticks(DvzAxis* axis, double min, double max, double step)
{
    ANN(axis);
    axis->tick_count = 0;
    if (!isfinite(min) || !isfinite(max) || !isfinite(step) || !(max > min) || step <= AXIS_EPS)
        return;

    double first = floor(min / step) * step;
    double last = ceil(max / step) * step;
    if (!isfinite(first) || !isfinite(last) || !(last >= first))
        return;
    for (double value = first; value <= last + 0.5 * step; value += step)
    {
        if (axis->tick_count >= DVZ_SCENE_MAX_AXIS_TICKS)
            break;
        axis->ticks[axis->tick_count++] = value;
    }
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
    if (axis->panel != NULL)
        (void)_scene_panel_panzoom_extent(axis->panel, extent);
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

    float pixel_span = 0.0f;
    uint32_t target = _axis_target_tick_count(axis, &pixel_span);
    double current_density = axis->tick_lstep > 0.0 && isfinite(axis->tick_lstep)
                                 ? (max - min) / axis->tick_lstep + 1.0
                                 : 0.0;
    if (!axis->dirty && axis->tick_cache_valid && min >= axis->tick_covered_min &&
        max <= axis->tick_covered_max &&
        current_density >= 0.5 * (double)target && current_density <= 1.5 * (double)target)
    {
        _axis_fill_visible_ticks(axis, min, max, axis->tick_lstep);
        return;
    }

    double range = max - min;
    if (!isfinite(range) || range <= AXIS_EPS)
        return;
    double raw_step = range / (double)(target - 1);
    double step = _axis_nice_number(raw_step, true);
    if (axis->tick_lstep > 0.0 && isfinite(axis->tick_lstep))
    {
        current_density = range / axis->tick_lstep + 1.0;
        if (current_density >= 0.5 * (double)target && current_density <= 1.5 * (double)target)
            step = axis->tick_lstep;
    }
    if (!isfinite(step) || step <= AXIS_EPS)
        return;

    const double margin_steps = 4.0;
    double visible_lmin = floor(min / step) * step;
    double visible_lmax = ceil(max / step) * step;
    double lmin = visible_lmin - margin_steps * step;
    double lmax = visible_lmax + margin_steps * step;
    if (!isfinite(lmin) || !isfinite(lmax) || !(lmax >= lmin))
        return;
    axis->tick_lmin = lmin;
    axis->tick_lmax = lmax;
    axis->tick_lstep = step;
    axis->tick_covered_min = lmin;
    axis->tick_covered_max = lmax;
    axis->tick_cache_valid = true;
    _axis_fill_visible_ticks(axis, min, max, step);
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
 */
static void _axis_append_line_rect(
    const DvzAxis* axis, uint32_t* count, float positions[][3], uint8_t colors[][4], float a0,
    float b0, float a1, float b1, float z, float width_px, const uint8_t color[4])
{
    ANN(axis);
    if (!(width_px > 0.0f) || !isfinite(width_px))
        return;
    float half_x = 0.5f * width_px * _axis_visual_pixel_size(axis, DVZ_DIM_X);
    float half_y = 0.5f * width_px * _axis_visual_pixel_size(axis, DVZ_DIM_Y);
    if (fabsf(a1 - a0) < fabsf(b1 - b0))
    {
        float x = _axis_snap_visual_pixel_center(axis, a0, DVZ_DIM_X);
        _axis_append_rect(count, positions, colors, x - half_x, b0, x + half_x, b1, z, color);
    }
    else
    {
        float y = _axis_snap_visual_pixel_center(axis, b0, DVZ_DIM_Y);
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
            axis, count, positions, colors, p, y0, p, y0 + length, z, width, color);
    else
        _axis_append_line_rect(
            axis, count, positions, colors, x0, p, x0 + length, p, z, width, color);
}



/**
 * Return whether the retained primitive visual already stores one axis geometry payload.
 *
 * @param axis the axis
 * @param vertex_count generated vertex count
 * @param positions generated vertex positions
 * @param colors generated vertex colors
 * @return whether the retained visual payload is unchanged
 */
static bool _axis_visual_cache_matches(
    const DvzAxis* axis, uint32_t vertex_count, const float* positions, const uint8_t* colors)
{
    ANN(axis);
    ANN(positions);
    ANN(colors);
    if (axis->visual == NULL || vertex_count == 0)
        return false;

    DvzVisualDataView position_view = {0};
    DvzVisualDataView color_view = {0};
    if (dvz_visual_data(axis->visual, "position", &position_view) != 0 ||
        dvz_visual_data(axis->visual, "color", &color_view) != 0)
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



/**
 * Rebuild the derived text visual for tick labels and the axis label.
 *
 * @param axis the axis
 * @param x0 plot left in visual coordinates
 * @param x1 plot right in visual coordinates
 * @param y0 plot bottom in visual coordinates
 * @param y1 plot top in visual coordinates
 * @param visible_min visible data minimum
 * @param visible_max visible data maximum
 */
static void _axis_update_text(
    DvzAxis* axis, float x0, float x1, float y0, float y1, double visible_min,
    double visible_max)
{
    ANN(axis);
    if (!axis->enabled || axis->tick_count == 0 || !(visible_max > visible_min))
    {
        _axis_hide_text(axis);
        return;
    }

    uint32_t count = 0;
    char labels[DVZ_SCENE_MAX_AXIS_TICKS + 1][DVZ_SCENE_LABEL_SIZE] = {{0}};
    const char* strings[DVZ_SCENE_MAX_AXIS_TICKS + 1] = {0};
    float positions[DVZ_SCENE_MAX_AXIS_TICKS + 1][3] = {{0}};
    float anchors[DVZ_SCENE_MAX_AXIS_TICKS + 1][2] = {{0}};
    float sizes[DVZ_SCENE_MAX_AXIS_TICKS + 1] = {0};
    uint8_t colors[DVZ_SCENE_MAX_AXIS_TICKS + 1][4] = {{0}};
    float angles[DVZ_SCENE_MAX_AXIS_TICKS + 1] = {0};

    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        float plot_min = axis->dim == DVZ_DIM_X ? x0 : y0;
        float plot_max = axis->dim == DVZ_DIM_X ? x1 : y1;
        float p =
            _axis_data_to_visual(axis->ticks[i], visible_min, visible_max, plot_min, plot_max);
        if (p < plot_min - 0.0001f || p > plot_max + 0.0001f)
            continue;

        char tick_label[DVZ_SCENE_LABEL_SIZE] = {0};
        _axis_format_tick(axis->ticks[i], axis->tick_lstep, tick_label, sizeof(tick_label));
        float px = 0.0f;
        float py = 0.0f;
        if (axis->dim == DVZ_DIM_X)
        {
            _axis_visual_to_pixels(axis, p, y0, &px, &py);
            py += axis->style.tick_gap_px > 0.0f && isfinite(axis->style.tick_gap_px) ?
                      axis->style.tick_gap_px :
                      AXIS_TEXT_TICK_GAP;
            _axis_append_text_item(
                &count, labels, strings, positions, anchors, sizes, colors, angles, tick_label, px,
                py, 0.5f, 0.0f,
                _axis_text_size(axis->style.tick_size_px, AXIS_TEXT_TICK_SIZE),
                axis->style.major_tick_color, 0.0f);
        }
        else
        {
            _axis_visual_to_pixels(axis, x0, p, &px, &py);
            px -= axis->style.tick_gap_px > 0.0f && isfinite(axis->style.tick_gap_px) ?
                      axis->style.tick_gap_px :
                      AXIS_TEXT_TICK_GAP;
            _axis_append_text_item(
                &count, labels, strings, positions, anchors, sizes, colors, angles, tick_label, px,
                py, 1.0f, 0.5f,
                _axis_text_size(axis->style.tick_size_px, AXIS_TEXT_TICK_SIZE),
                axis->style.major_tick_color, 0.0f);
        }
    }

    if (axis->label[0] != '\0')
    {
        float px = 0.0f;
        float py = 0.0f;
        if (axis->dim == DVZ_DIM_X)
        {
            _axis_visual_to_pixels(axis, 0.5f * (x0 + x1), y0, &px, &py);
            py += axis->style.label_gap_px > 0.0f && isfinite(axis->style.label_gap_px) ?
                      axis->style.label_gap_px :
                      AXIS_TEXT_LABEL_GAP;
            _axis_append_text_item(
                &count, labels, strings, positions, anchors, sizes, colors, angles, axis->label,
                px, py, 0.5f, 0.0f,
                _axis_text_size(axis->style.label_size_px, AXIS_TEXT_LABEL_SIZE),
                axis->style.spine_color, 0.0f);
        }
        else
        {
            _axis_visual_to_pixels(axis, x0, 0.5f * (y0 + y1), &px, &py);
            px -= axis->style.label_gap_px > 0.0f && isfinite(axis->style.label_gap_px) ?
                      axis->style.label_gap_px :
                      AXIS_TEXT_LABEL_GAP;
            _axis_append_text_item(
                &count, labels, strings, positions, anchors, sizes, colors, angles, axis->label,
                px, py, 0.5f, 0.5f,
                _axis_text_size(axis->style.label_size_px, AXIS_TEXT_LABEL_SIZE),
                axis->style.spine_color, AXIS_TEXT_Y_LABEL_ANGLE);
        }
    }

    if (count == 0)
    {
        _axis_hide_text(axis);
        return;
    }
    if (!_axis_ensure_text_visual(axis))
        return;
    if (!_axis_apply_text_renderer(axis))
        return;
    if (_axis_text_cache_matches(axis, count, labels, positions, anchors, sizes, colors, angles))
    {
        if (!axis->text_visual->visible)
            dvz_visual_set_visible(axis->text_visual, true);
        return;
    }

    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "anchor", .data = anchors, .item_count = count},
        {.attr_name = "size", .data = sizes, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "angle", .data = angles, .item_count = count},
    };
    if (dvz_visual_set_strings(axis->text_visual, "text", strings, count) == 0 &&
        dvz_visual_set_data_many(axis->text_visual, updates, 5) == 0)
    {
        if (!axis->text_visual->visible)
            dvz_visual_set_visible(axis->text_visual, true);
        _axis_text_cache_store(axis, count, labels, positions, anchors, sizes, colors, angles);
    }
    else
    {
        _axis_hide_text(axis);
    }
}



/**
 * Rebuild the fixed-space primitive visual backing one axis.
 *
 * @param axis the axis
 */
static void _axis_update_visual(DvzAxis* axis)
{
    ANN(axis);
    if (axis->visual == NULL)
        return;

    uint32_t vertex_count = 0;
    float positions[6 * DVZ_SCENE_MAX_AXIS_LINES][3] = {{0}};
    uint8_t colors[6 * DVZ_SCENE_MAX_AXIS_LINES][4] = {{0}};
    const float z = 0.0f;
    float x0 = -1.0f;
    float x1 = +1.0f;
    float y0 = -1.0f;
    float y1 = +1.0f;
    _axis_init(&axis->panel->axes[DVZ_DIM_X], axis->panel, DVZ_DIM_X);
    _axis_init(&axis->panel->axes[DVZ_DIM_Y], axis->panel, DVZ_DIM_Y);
    _axis_plot_interval(&axis->panel->axes[DVZ_DIM_X], &x0, &x1);
    _axis_plot_interval(&axis->panel->axes[DVZ_DIM_Y], &y0, &y1);

    double visible_min = 0.0;
    double visible_max = 0.0;
    if (!_axis_visible_domain(axis, &visible_min, &visible_max))
    {
        _axis_hide_text(axis);
        return;
    }
    _axis_compute_ticks(axis);

    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        float plot_min = axis->dim == DVZ_DIM_X ? x0 : y0;
        float plot_max = axis->dim == DVZ_DIM_X ? x1 : y1;
        float p =
            _axis_data_to_visual(axis->ticks[i], visible_min, visible_max, plot_min, plot_max);
        if (p < plot_min - 0.0001f || p > plot_max + 0.0001f)
            continue;
        bool boundary_grid = fabsf(p - plot_min) <= 0.0001f || fabsf(p - plot_max) <= 0.0001f;
        if (axis->style.show_grid && !(axis->style.show_spine && boundary_grid))
        {
            if (axis->dim == DVZ_DIM_X)
                _axis_append_line_rect(
                    axis, &vertex_count, positions, colors, p, y0, p, y1, z,
                    axis->style.grid_width, axis->style.grid_color);
            else
                _axis_append_line_rect(
                    axis, &vertex_count, positions, colors, x0, p, x1, p, z,
                    axis->style.grid_width, axis->style.grid_color);
        }
    }

    if (axis->style.show_spine)
    {
        if (axis->dim == DVZ_DIM_X)
        {
            float y = y0 + 0.5f * axis->style.spine_width *
                               _axis_visual_pixel_size(axis, DVZ_DIM_Y);
            _axis_append_line_rect(
                axis, &vertex_count, positions, colors, x0, y, x1, y, z, axis->style.spine_width,
                axis->style.spine_color);
        }
        else
        {
            float x = x0 + 0.5f * axis->style.spine_width *
                               _axis_visual_pixel_size(axis, DVZ_DIM_X);
            _axis_append_line_rect(
                axis, &vertex_count, positions, colors, x, y0, x, y1, z, axis->style.spine_width,
                axis->style.spine_color);
        }
    }

    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        float plot_min = axis->dim == DVZ_DIM_X ? x0 : y0;
        float plot_max = axis->dim == DVZ_DIM_X ? x1 : y1;
        float p =
            _axis_data_to_visual(axis->ticks[i], visible_min, visible_max, plot_min, plot_max);
        if (p < plot_min - 0.0001f || p > plot_max + 0.0001f)
            continue;
        if (axis->style.show_major_ticks)
        {
            float len = _axis_tick_length(axis, axis->style.major_tick_length);
            _axis_append_tick(
                axis, &vertex_count, positions, colors, p, x0, y0, z, len,
                axis->style.major_tick_color, axis->style.major_tick_width);
        }
        if (axis->style.show_minor_ticks && i + 1 < axis->tick_count)
        {
            uint32_t minor_count = _axis_minor_count(axis);
            float len = _axis_tick_length(axis, axis->style.minor_tick_length);
            double delta = (axis->ticks[i + 1] - axis->ticks[i]) / (double)(minor_count + 1);
            for (uint32_t j = 1; j <= minor_count; j++)
            {
                double value = axis->ticks[i] + (double)j * delta;
                float mp =
                    _axis_data_to_visual(value, visible_min, visible_max, plot_min, plot_max);
                if (mp < plot_min - 0.0001f || mp > plot_max + 0.0001f)
                    continue;
                _axis_append_tick(
                    axis, &vertex_count, positions, colors, mp, x0, y0, z, len,
                    axis->style.minor_tick_color, axis->style.minor_tick_width);
            }
        }
    }

    axis->visual->visible = axis->enabled && vertex_count > 0;
    if (vertex_count == 0)
    {
        _axis_hide_text(axis);
        return;
    }
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
    };
    if (!_axis_visual_cache_matches(axis, vertex_count, &positions[0][0], &colors[0][0]))
        (void)dvz_visual_set_data_many(axis->visual, updates, 2);
    _axis_update_text(axis, x0, x1, y0, y1, visible_min, visible_max);
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
    axis->domain_set = true;
    axis->tick_lstep = 0.0;
    axis->tick_cache_valid = false;
    axis->dirty = true;
    axis->version++;
    _scene_notify_request_frame(panel->figure);
    return 0;
}


/**
 * Return the current visible data domain for one panel dimension.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @param out_min output visible data minimum
 * @param out_max output visible data maximum
 * @return whether the visible domain was written
 */
bool dvz_panel_visible_domain(DvzPanel* panel, DvzDim dim, double* out_min, double* out_max)
{
    DvzAxis* axis = _panel_axis_slot(panel, dim);
    if (axis == NULL || out_min == NULL || out_max == NULL)
        return false;
    _axis_init(axis, panel, dim);
    if (!axis->domain_set)
    {
        float extent[4] = {-1.0f, +1.0f, -1.0f, +1.0f};
        (void)_scene_panel_panzoom_extent(panel, extent);
        uint32_t lo_idx = dim == DVZ_DIM_X ? 0 : 2;
        uint32_t hi_idx = dim == DVZ_DIM_X ? 1 : 3;
        *out_min = fmin((double)extent[lo_idx], (double)extent[hi_idx]);
        *out_max = fmax((double)extent[lo_idx], (double)extent[hi_idx]);
        return isfinite(*out_min) && isfinite(*out_max) && *out_max > *out_min;
    }
    return _axis_visible_domain(axis, out_min, out_max);
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
    bool has_x = x_axis != NULL && x_axis->panel != NULL && x_axis->domain_set;
    bool has_y = y_axis != NULL && y_axis->panel != NULL && y_axis->domain_set;
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
 * Return a panel-owned axis, creating its fixed primitive visual on first use.
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
        axis->visual =
            dvz_primitive(panel->figure->scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
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
    _axis_mark_dirty(axis);
    return axis;
}


/**
 * Return the default axis tick policy.
 *
 * @return default axis tick policy
 */
DvzAxisTickPolicy dvz_axis_tick_policy(void)
{
    return _axis_default_tick_policy();
}


/**
 * Return the default axis line and text style.
 *
 * @return default axis style
 */
DvzAxisStyle dvz_axis_style(void)
{
    return _axis_default_style();
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
    if (axis->visual != NULL && !visible)
        axis->visual->visible = false;
    if (!visible)
        _axis_hide_text(axis);
    _axis_mark_dirty(axis);
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
    _axis_mark_dirty(axis);
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
    _axis_mark_dirty(axis);
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
    _axis_mark_dirty(axis);
    return true;
}



/**
 * Set the line and text style for one panel-owned axis.
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
    _axis_mark_dirty(axis);
    return true;
}


/**
 * Set plot-area margins for one panel-owned axis.
 *
 * @param axis the axis
 * @param left left margin
 * @param right right margin
 * @param bottom bottom margin
 * @param top top margin
 * @return whether the margins were updated
 */
bool dvz_axis_set_plot_margins(
    DvzAxis* axis, float left, float right, float bottom, float top)
{
    if (axis == NULL)
        return false;
    if (!isfinite(left) || !isfinite(right) || !isfinite(bottom) || !isfinite(top) ||
        left < 0.0f || right < 0.0f || bottom < 0.0f || top < 0.0f ||
        left + right >= 2.0f || bottom + top >= 2.0f)
        return false;
    axis->style.plot_margin_left = left;
    axis->style.plot_margin_right = right;
    axis->style.plot_margin_bottom = bottom;
    axis->style.plot_margin_top = top;
    _axis_mark_dirty(axis);
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
        _scene_panel_refresh_axis_reserve(panel);
        for (uint32_t dim = 0; dim < 2; dim++)
        {
            DvzAxis* axis = &panel->axes[dim];
            if (axis->panel == NULL || axis->visual == NULL)
                continue;
            _axis_update_visual(axis);
        }
    }
}
