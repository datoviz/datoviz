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
#include "core/units_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define AXIS_TIME_US_PER_MILLISECOND 1000LL
#define AXIS_TIME_US_PER_SECOND      1000000LL
#define AXIS_TIME_US_PER_MINUTE      (60LL * AXIS_TIME_US_PER_SECOND)
#define AXIS_TIME_US_PER_HOUR        (60LL * AXIS_TIME_US_PER_MINUTE)
#define AXIS_TIME_US_PER_DAY         (24LL * AXIS_TIME_US_PER_HOUR)


typedef struct DvzAxisTimeStep
{
    DvzTimeInterval interval;
    DvzTimestamp step_us;
    int32_t calendar_months;
    int32_t calendar_years;
} DvzAxisTimeStep;


static int64_t _axis_floor_div_i64(int64_t value, int64_t divisor)
{
    ASSERT(divisor != 0);
    int64_t q = value / divisor;
    int64_t r = value % divisor;
    if (r != 0 && ((r < 0) != (divisor < 0)))
        q--;
    return q;
}


static DvzTimestamp _axis_timestamp_floor(DvzTimestamp timestamp, DvzTimestamp step_us)
{
    if (step_us <= 0)
        return timestamp;
    return (DvzTimestamp)(_axis_floor_div_i64(timestamp, step_us) * step_us);
}


static int64_t _axis_days_from_civil(int32_t year, uint32_t month, uint32_t day)
{
    year -= month <= 2u ? 1 : 0;
    const int32_t era = (year >= 0 ? year : year - 399) / 400;
    const uint32_t yoe = (uint32_t)(year - era * 400);
    const uint32_t doy =
        (153u * (month + (month > 2u ? (uint32_t)-3 : 9u)) + 2u) / 5u + day - 1u;
    const uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return (int64_t)era * 146097LL + (int64_t)doe - 719468LL;
}


static void _axis_civil_from_days(int64_t days, int32_t* out_year, uint32_t* out_month)
{
    ANN(out_year);
    ANN(out_month);
    days += 719468LL;
    const int64_t era = (days >= 0 ? days : days - 146096LL) / 146097LL;
    const uint32_t doe = (uint32_t)(days - era * 146097LL);
    const uint32_t yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    int32_t year = (int32_t)yoe + (int32_t)era * 400;
    const uint32_t doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    const uint32_t mp = (5u * doy + 2u) / 153u;
    const uint32_t day = doy - (153u * mp + 2u) / 5u + 1u;
    (void)day;
    const uint32_t month = mp + (mp < 10u ? 3u : (uint32_t)-9);
    year += month <= 2u ? 1 : 0;
    *out_year = year;
    *out_month = month;
}


static DvzTimestamp _axis_timestamp_month_floor(DvzTimestamp timestamp, int32_t months)
{
    if (months <= 0)
        return timestamp;
    int64_t days = _axis_floor_div_i64(timestamp, AXIS_TIME_US_PER_DAY);
    int32_t year = 1970;
    uint32_t month = 1;
    _axis_civil_from_days(days, &year, &month);
    int64_t month_index = (int64_t)year * 12LL + (int64_t)month - 1LL;
    month_index = _axis_floor_div_i64(month_index, months) * (int64_t)months;
    year = (int32_t)_axis_floor_div_i64(month_index, 12);
    month = (uint32_t)(month_index - (int64_t)year * 12LL) + 1u;
    return (DvzTimestamp)(_axis_days_from_civil(year, month, 1u) * AXIS_TIME_US_PER_DAY);
}


static DvzTimestamp _axis_timestamp_year_floor(DvzTimestamp timestamp, int32_t years)
{
    if (years <= 0)
        return timestamp;
    int64_t days = _axis_floor_div_i64(timestamp, AXIS_TIME_US_PER_DAY);
    int32_t year = 1970;
    uint32_t month = 1;
    _axis_civil_from_days(days, &year, &month);
    (void)month;
    year = (int32_t)(_axis_floor_div_i64(year, years) * years);
    return (DvzTimestamp)(_axis_days_from_civil(year, 1u, 1u) * AXIS_TIME_US_PER_DAY);
}


static DvzTimestamp _axis_timestamp_add_months(DvzTimestamp timestamp, int32_t months)
{
    int64_t days = _axis_floor_div_i64(timestamp, AXIS_TIME_US_PER_DAY);
    int32_t year = 1970;
    uint32_t month = 1;
    _axis_civil_from_days(days, &year, &month);
    int64_t month_index = (int64_t)year * 12LL + (int64_t)month - 1LL + (int64_t)months;
    year = (int32_t)_axis_floor_div_i64(month_index, 12);
    month = (uint32_t)(month_index - (int64_t)year * 12LL) + 1u;
    return (DvzTimestamp)(_axis_days_from_civil(year, month, 1u) * AXIS_TIME_US_PER_DAY);
}


static DvzTimestamp _axis_timestamp_add_years(DvzTimestamp timestamp, int32_t years)
{
    int64_t days = _axis_floor_div_i64(timestamp, AXIS_TIME_US_PER_DAY);
    int32_t year = 1970;
    uint32_t month = 1;
    _axis_civil_from_days(days, &year, &month);
    (void)month;
    return (DvzTimestamp)(_axis_days_from_civil(year + years, 1u, 1u) * AXIS_TIME_US_PER_DAY);
}


static DvzAxisTimeStep _axis_datetime_choose_step(double raw_step_us)
{
    static const DvzAxisTimeStep steps[] = {
        {DVZ_TIME_INTERVAL_MICROSECOND, 1, 0, 0},
        {DVZ_TIME_INTERVAL_MICROSECOND, 2, 0, 0},
        {DVZ_TIME_INTERVAL_MICROSECOND, 5, 0, 0},
        {DVZ_TIME_INTERVAL_MICROSECOND, 10, 0, 0},
        {DVZ_TIME_INTERVAL_MICROSECOND, 20, 0, 0},
        {DVZ_TIME_INTERVAL_MICROSECOND, 50, 0, 0},
        {DVZ_TIME_INTERVAL_MICROSECOND, 100, 0, 0},
        {DVZ_TIME_INTERVAL_MICROSECOND, 200, 0, 0},
        {DVZ_TIME_INTERVAL_MICROSECOND, 500, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 1 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 2 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 5 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 10 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 20 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 50 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 100 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 200 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MILLISECOND, 500 * AXIS_TIME_US_PER_MILLISECOND, 0, 0},
        {DVZ_TIME_INTERVAL_SECOND, 1 * AXIS_TIME_US_PER_SECOND, 0, 0},
        {DVZ_TIME_INTERVAL_SECOND, 2 * AXIS_TIME_US_PER_SECOND, 0, 0},
        {DVZ_TIME_INTERVAL_SECOND, 5 * AXIS_TIME_US_PER_SECOND, 0, 0},
        {DVZ_TIME_INTERVAL_SECOND, 10 * AXIS_TIME_US_PER_SECOND, 0, 0},
        {DVZ_TIME_INTERVAL_SECOND, 15 * AXIS_TIME_US_PER_SECOND, 0, 0},
        {DVZ_TIME_INTERVAL_SECOND, 30 * AXIS_TIME_US_PER_SECOND, 0, 0},
        {DVZ_TIME_INTERVAL_MINUTE, 1 * AXIS_TIME_US_PER_MINUTE, 0, 0},
        {DVZ_TIME_INTERVAL_MINUTE, 2 * AXIS_TIME_US_PER_MINUTE, 0, 0},
        {DVZ_TIME_INTERVAL_MINUTE, 5 * AXIS_TIME_US_PER_MINUTE, 0, 0},
        {DVZ_TIME_INTERVAL_MINUTE, 10 * AXIS_TIME_US_PER_MINUTE, 0, 0},
        {DVZ_TIME_INTERVAL_MINUTE, 15 * AXIS_TIME_US_PER_MINUTE, 0, 0},
        {DVZ_TIME_INTERVAL_MINUTE, 30 * AXIS_TIME_US_PER_MINUTE, 0, 0},
        {DVZ_TIME_INTERVAL_HOUR, 1 * AXIS_TIME_US_PER_HOUR, 0, 0},
        {DVZ_TIME_INTERVAL_HOUR, 2 * AXIS_TIME_US_PER_HOUR, 0, 0},
        {DVZ_TIME_INTERVAL_HOUR, 3 * AXIS_TIME_US_PER_HOUR, 0, 0},
        {DVZ_TIME_INTERVAL_HOUR, 6 * AXIS_TIME_US_PER_HOUR, 0, 0},
        {DVZ_TIME_INTERVAL_HOUR, 12 * AXIS_TIME_US_PER_HOUR, 0, 0},
        {DVZ_TIME_INTERVAL_DAY, 1 * AXIS_TIME_US_PER_DAY, 0, 0},
        {DVZ_TIME_INTERVAL_DAY, 2 * AXIS_TIME_US_PER_DAY, 0, 0},
        {DVZ_TIME_INTERVAL_DAY, 7 * AXIS_TIME_US_PER_DAY, 0, 0},
        {DVZ_TIME_INTERVAL_DAY, 14 * AXIS_TIME_US_PER_DAY, 0, 0},
        {DVZ_TIME_INTERVAL_MONTH, 0, 1, 0},
        {DVZ_TIME_INTERVAL_MONTH, 0, 2, 0},
        {DVZ_TIME_INTERVAL_MONTH, 0, 3, 0},
        {DVZ_TIME_INTERVAL_MONTH, 0, 6, 0},
        {DVZ_TIME_INTERVAL_YEAR, 0, 0, 1},
        {DVZ_TIME_INTERVAL_YEAR, 0, 0, 2},
        {DVZ_TIME_INTERVAL_YEAR, 0, 0, 5},
        {DVZ_TIME_INTERVAL_YEAR, 0, 0, 10},
    };
    static const double approx[] = {
        1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100.0, 200.0, 500.0,
        1e3, 2e3, 5e3, 10e3, 20e3, 50e3, 100e3, 200e3, 500e3,
        1e6, 2e6, 5e6, 10e6, 15e6, 30e6,
        60e6, 120e6, 300e6, 600e6, 900e6, 1800e6,
        3600e6, 7200e6, 10800e6, 21600e6, 43200e6,
        86400e6, 172800e6, 604800e6, 1209600e6,
        30.0 * 86400e6, 60.0 * 86400e6, 90.0 * 86400e6, 180.0 * 86400e6,
        365.0 * 86400e6, 2.0 * 365.0 * 86400e6, 5.0 * 365.0 * 86400e6,
        10.0 * 365.0 * 86400e6,
    };
    uint32_t count = sizeof(steps) / sizeof(steps[0]);
    for (uint32_t i = 0; i < count; i++)
    {
        if (raw_step_us <= approx[i])
            return steps[i];
    }
    return steps[count - 1];
}


static DvzTimestamp _axis_datetime_step_floor(DvzTimestamp timestamp, DvzAxisTimeStep step)
{
    if (step.calendar_years > 0)
        return _axis_timestamp_year_floor(timestamp, step.calendar_years);
    if (step.calendar_months > 0)
        return _axis_timestamp_month_floor(timestamp, step.calendar_months);
    return _axis_timestamp_floor(timestamp, step.step_us);
}


static DvzTimestamp _axis_datetime_step_next(DvzTimestamp timestamp, DvzAxisTimeStep step)
{
    if (step.calendar_years > 0)
        return _axis_timestamp_add_years(timestamp, step.calendar_years);
    if (step.calendar_months > 0)
        return _axis_timestamp_add_months(timestamp, step.calendar_months);
    return timestamp + step.step_us;
}


static void _axis_compute_datetime_ticks(
    DvzAxis* axis, double min, double max, uint32_t target)
{
    ANN(axis);
    if (axis->datetime_format == NULL || !axis->datetime_range_set || !(max > min))
        return;

    DvzTimestamp a = _scene_datetime_data_to_timestamp(axis, min);
    DvzTimestamp b = _scene_datetime_data_to_timestamp(axis, max);
    DvzTimestamp lo = a <= b ? a : b;
    DvzTimestamp hi = a <= b ? b : a;
    if (hi <= lo)
        return;

    double raw_step_us = (double)(hi - lo) / (double)(target > 1 ? target - 1 : 1);
    if (!isfinite(raw_step_us) || raw_step_us <= 0.0)
        return;
    DvzAxisTimeStep step = _axis_datetime_choose_step(raw_step_us);
    axis->datetime_tick_interval = step.interval;

    DvzTimestamp tick = _axis_datetime_step_floor(lo, step);
    while (tick < lo)
    {
        DvzTimestamp next = _axis_datetime_step_next(tick, step);
        if (next <= tick)
            return;
        tick = next;
    }

    axis->tick_count = 0;
    DvzTimestamp previous = tick;
    while (tick <= hi && axis->tick_count < DVZ_SCENE_MAX_AXIS_TICKS)
    {
        double data = _scene_datetime_timestamp_to_data(axis, tick);
        if (isfinite(data))
            axis->ticks[axis->tick_count++] = data;
        DvzTimestamp next = _axis_datetime_step_next(tick, step);
        if (next <= tick)
            break;
        previous = tick;
        tick = next;
    }

    if (axis->tick_count >= 2)
        axis->tick_lstep = fabs(axis->ticks[1] - axis->ticks[0]);
    else if (axis->tick_count == 1)
        axis->tick_lstep =
            fabs(_scene_datetime_timestamp_to_data(axis, _axis_datetime_step_next(previous, step)) -
                 axis->ticks[0]);
    else
        axis->tick_lstep = 0.0;
    axis->tick_lmin = axis->tick_count > 0 ? axis->ticks[0] : min;
    axis->tick_lmax = axis->tick_count > 0 ? axis->ticks[axis->tick_count - 1] : max;
    axis->tick_covered_min = min;
    axis->tick_covered_max = max;
    axis->tick_cache_valid = false;
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


float _axis_forward_panzoom_coord(
    const float extent[4], uint32_t lo_idx, uint32_t hi_idx, float value)
{
    ANN(extent);
    float range = extent[hi_idx] - extent[lo_idx];
    if (fabsf(range) <= 1e-12f || !isfinite(range))
        return value;
    return 2.0f * (value - 0.5f * (extent[lo_idx] + extent[hi_idx])) / range;
}


float _axis_data_to_source_visual(const DvzAxis* axis, double value)
{
    ANN(axis);
    if (axis->panel != NULL && axis->panel->view2d_enabled)
    {
        mat4 data_to_view = GLM_MAT4_IDENTITY_INIT;
        if (_scene_panel_data_model(axis->panel, data_to_view))
        {
            uint32_t dim = axis->dim == DVZ_DIM_X ? 0 : 1;
            return data_to_view[dim][dim] * (float)value + data_to_view[3][dim];
        }
    }
    if (!axis->domain_set)
        return (float)value;
    return _axis_data_to_visual(value, axis->domain.min, axis->domain.max, -1.0f, +1.0f);
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
    double denom = 2.0;
    if (fabs(denom) < AXIS_EPS)
        return axis->domain.min;
    double t = ((double)value + 1.0) / denom;
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
    float a_view = _axis_inverse_panzoom_coord(extent, lo_idx, hi_idx, -1.0f);
    float b_view = _axis_inverse_panzoom_coord(extent, lo_idx, hi_idx, +1.0f);
    if (axis->panel != NULL && axis->panel->view2d_enabled)
    {
        a_view = extent[lo_idx];
        b_view = extent[hi_idx];
        DvzPanelView2DResolved resolved = {0};
        if (_scene_panel_view2d_resolve(axis->panel, &resolved))
        {
            double data_min = axis->dim == DVZ_DIM_X ? resolved.data_x[0] : resolved.data_y[0];
            double data_max = axis->dim == DVZ_DIM_X ? resolved.data_x[1] : resolved.data_y[1];
            double view_min = (double)resolved.view_extent[lo_idx];
            double view_max = (double)resolved.view_extent[hi_idx];
            double scale = (view_max - view_min) / (data_max - data_min);
            double translate = view_min - scale * data_min;
            if (isfinite(scale) && fabs(scale) >= AXIS_EPS && isfinite(translate))
            {
                double a = ((double)a_view - translate) / scale;
                double b = ((double)b_view - translate) / scale;
                *out_min = a;
                *out_max = b;
                return isfinite(*out_min) && isfinite(*out_max) &&
                       fabs(*out_max - *out_min) > AXIS_EPS;
            }
        }
    }
    if (!axis->domain_set)
    {
        *out_min = (double)a_view;
        *out_max = (double)b_view;
        return isfinite(*out_min) && isfinite(*out_max) &&
               fabs(*out_max - *out_min) > AXIS_EPS;
    }

    double a = _axis_visual_to_data(axis, a_view);
    double b = _axis_visual_to_data(axis, b_view);
    *out_min = a;
    *out_max = b;
    return isfinite(*out_min) && isfinite(*out_max) &&
           fabs(*out_max - *out_min) > AXIS_EPS;
}


bool _axis_visible_sorted_interval(const DvzAxis* axis, double* out_min, double* out_max)
{
    ANN(axis);
    ANN(out_min);
    ANN(out_max);
    double a = 0.0;
    double b = 0.0;
    if (!_axis_visible_domain(axis, &a, &b))
        return false;
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
    if (!_axis_visible_sorted_interval(axis, &min, &max))
        return;

    float pixel_span = 0.0f;
    uint32_t target = _axis_target_tick_count(axis, &pixel_span);
    if (axis->datetime_format != NULL && axis->datetime_range_set)
    {
        _axis_compute_datetime_ticks(axis, min, max, target);
        return;
    }

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
