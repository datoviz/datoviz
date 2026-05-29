/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene axis tick and domain helpers                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "axis_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
 * Map one fixed panel coordinate through the inverse panzoom extent.
 *
 * @param extent full-panel inverse panzoom extent as xmin, xmax, ymin, ymax
 * @param lo_idx extent minimum index for the axis dimension
 * @param hi_idx extent maximum index for the axis dimension
 * @param value fixed panel coordinate in [-1, +1]
 * @return untransformed visual coordinate
 */
float _axis_inverse_panzoom_coord(
    const float extent[4], uint32_t lo_idx, uint32_t hi_idx, float value)
{
    ANN(extent);
    return 0.5f * (extent[lo_idx] + extent[hi_idx]) +
           0.5f * value * (extent[hi_idx] - extent[lo_idx]);
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
 * Return the active panzoom scale for one visual dimension.
 *
 * @param extent full-panel inverse panzoom extent as xmin, xmax, ymin, ymax
 * @param dim visual dimension
 * @return controller scale, or 1 when unavailable
 */
float _axis_panzoom_scale(const float extent[4], DvzDim dim)
{
    ANN(extent);
    uint32_t lo_idx = dim == DVZ_DIM_X ? 0 : 2;
    uint32_t hi_idx = dim == DVZ_DIM_X ? 1 : 3;
    float range = fabsf(extent[hi_idx] - extent[lo_idx]);
    if (!(range > 0.0f) || !isfinite(range))
        return 1.0f;
    float scale = 2.0f / range;
    return scale > 0.0f && isfinite(scale) ? scale : 1.0f;
}


/**
 * Return the visible data interval for an axis.
 *
 * @param axis the axis
 * @param out_min output data minimum
 * @param out_max output data maximum
 * @return whether the interval was written
 */
bool _axis_visible_domain(const DvzAxis* axis, double* out_min, double* out_max)
{
    ANN(axis);
    ANN(out_min);
    ANN(out_max);
    float extent[4] = {-1.0f, +1.0f, -1.0f, +1.0f};
    if (axis->panel != NULL)
        (void)_scene_panel_panzoom_extent(axis->panel, extent);
    uint32_t lo_idx = axis->dim == DVZ_DIM_X ? 0 : 2;
    uint32_t hi_idx = axis->dim == DVZ_DIM_X ? 1 : 3;
    float visual_min = -1.0f;
    float visual_max = +1.0f;
    _axis_plot_interval(axis, &visual_min, &visual_max);
    float a_visual = _axis_inverse_panzoom_coord(extent, lo_idx, hi_idx, visual_min);
    float b_visual = _axis_inverse_panzoom_coord(extent, lo_idx, hi_idx, visual_max);
    if (!axis->domain_set)
    {
        *out_min = fmin((double)a_visual, (double)b_visual);
        *out_max = fmax((double)a_visual, (double)b_visual);
        return isfinite(*out_min) && isfinite(*out_max) && *out_max > *out_min;
    }

    double a = _axis_visual_to_data(axis, a_visual);
    double b = _axis_visual_to_data(axis, b_visual);
    *out_min = fmin(a, b);
    *out_max = fmax(a, b);
    return isfinite(*out_min) && isfinite(*out_max) && *out_max > *out_min;
}


/**
 * Compute major tick values for one axis.
 *
 * @param axis the axis
 */
void _axis_compute_ticks(DvzAxis* axis)
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


