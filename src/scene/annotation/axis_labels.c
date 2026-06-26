/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene axis label planning                                                                    */
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
#include "axis_internal.h"
#include "axis_labels_internal.h"
#include "core/units_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define AXIS_OFFSET_THRESHOLD 10000.0



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static uint32_t _axis_decimal_places(double step)
{
    if (!isfinite(step) || step <= 0.0)
        return 6;
    double exponent = floor(log10(step));
    if (exponent >= 0.0)
        return 0;
    uint32_t decimals = (uint32_t)(-exponent) + 2;
    return decimals > 12 ? 12 : decimals;
}


static void _axis_trim_decimal(char* text)
{
    ANN(text);
    char* dot = strchr(text, '.');
    if (dot == NULL)
        return;
    char* end = text + strlen(text);
    while (end > dot + 1 && end[-1] == '0')
    {
        end--;
        *end = '\0';
    }
    if (end > dot && end[-1] == '.')
        end[-1] = '\0';
}


static void _axis_format_compact_number(double value, double step, char* out, uint32_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return;
    if (!isfinite(value))
    {
        dvz_snprintf(out, out_size, "%.6g", value);
        return;
    }
    if (isfinite(step) && step > 0.0 && fabs(value) < 0.5 * step * 1e-9)
        value = 0.0;
    uint32_t decimals = _axis_decimal_places(fabs(step));
    if (decimals == 0)
        dvz_snprintf(out, out_size, "%.0f", value);
    else
    {
        dvz_snprintf(out, out_size, "%.*f", (int)decimals, value);
        _axis_trim_decimal(out);
    }
}


static void _axis_format_numeric_tick(double value, double step, char* out, uint32_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return;
    if (isfinite(step) && step > 0.0 && fabs(value) < 0.5 * step * 1e-9)
        value = 0.0;
    dvz_snprintf(out, out_size, "%.6g", value);
}


static bool _axis_numeric_offset(
    const double* ticks, uint32_t tick_count, double visible_min, double visible_max,
    double* out_offset)
{
    ANN(out_offset);
    if (ticks == NULL || tick_count < 2 || !isfinite(visible_min) || !isfinite(visible_max) ||
        !(visible_max > visible_min))
        return false;
    double range = visible_max - visible_min;
    double center = 0.5 * (visible_min + visible_max);
    if (!isfinite(range) || range <= 0.0 || !isfinite(center) ||
        fabs(center) < AXIS_OFFSET_THRESHOLD * range)
        return false;

    double offset = ticks[0];
    if (!isfinite(offset) || fabs(offset) <= AXIS_EPS)
        return false;
    *out_offset = offset;
    return true;
}


static void _axis_format_tick_label(
    const DvzAxis* axis, double value, double visible_min, double visible_max, char* out,
    uint32_t out_size)
{
    ANN(axis);
    ANN(out);
    if (out_size == 0)
        return;
    if (axis->datetime_format != NULL && axis->datetime_range_set)
    {
        DvzTimestamp timestamp = _scene_datetime_data_to_timestamp(axis, value);
        if (_scene_datetime_format(
                axis->datetime_format, timestamp, axis->datetime_tick_interval, out, out_size))
            return;
    }
    if (axis->units != NULL)
    {
        DvzUnitFormatContext context = {
            .mode = DVZ_UNIT_DISPLAY_AXIS_STABLE,
            .has_axis_range = true,
            .axis_data_min = visible_min,
            .axis_data_max = visible_max,
        };
        if (_scene_units_format(axis->units, value, &context, out, out_size))
            return;
    }
    _axis_format_numeric_tick(value, axis->tick_lstep, out, out_size);
}


static bool _axis_label_plan_datetime(
    const DvzAxis* axis, const double* ticks, uint32_t tick_count, double visible_min,
    double visible_max, DvzAxisLabelPlan* out)
{
    ANN(axis);
    ANN(ticks);
    ANN(out);
    if (axis->datetime_format == NULL || !axis->datetime_range_set ||
        axis->datetime_tick_interval >= DVZ_TIME_INTERVAL_DAY || tick_count < 2)
        return false;

    for (uint32_t i = 0; i < tick_count; i++)
    {
        DvzTimestamp timestamp = _scene_datetime_data_to_timestamp(axis, ticks[i]);
        if (!_scene_datetime_format(
                axis->datetime_format, timestamp, axis->datetime_tick_interval,
                out->tick_labels[i], sizeof(out->tick_labels[i])))
            return false;
    }

    DvzTimestamp t0 = _scene_datetime_data_to_timestamp(axis, visible_min);
    DvzTimestamp t1 = _scene_datetime_data_to_timestamp(axis, visible_max);
    if (t1 < t0)
    {
        DvzTimestamp tmp = t0;
        t0 = t1;
        t1 = tmp;
    }
    char context0[DVZ_SCENE_LABEL_SIZE] = {0};
    char context1[DVZ_SCENE_LABEL_SIZE] = {0};
    if (!_scene_datetime_format(
            axis->datetime_format, t0, DVZ_TIME_INTERVAL_DAY, context0, sizeof(context0)) ||
        !_scene_datetime_format(
            axis->datetime_format, t1, DVZ_TIME_INTERVAL_DAY, context1, sizeof(context1)))
        return false;

    out->has_offset_label = true;
    if (strcmp(context0, context1) == 0)
        dvz_strlcpy(out->offset_label, context0, sizeof(out->offset_label));
    else
        dvz_snprintf(
            out->offset_label, sizeof(out->offset_label), "%s - %s", context0, context1);
    return true;
}


static bool _axis_label_plan_numeric_offset(
    const DvzAxis* axis, const double* ticks, uint32_t tick_count, double visible_min,
    double visible_max, DvzAxisLabelPlan* out)
{
    ANN(axis);
    ANN(ticks);
    ANN(out);
    double offset = 0.0;
    if (!_axis_numeric_offset(ticks, tick_count, visible_min, visible_max, &offset))
        return false;

    double step = tick_count >= 2 ? fabs(ticks[1] - ticks[0]) : axis->tick_lstep;
    if (axis->units != NULL)
    {
        DvzUnitFormatContext offset_context = {
            .mode = DVZ_UNIT_DISPLAY_AXIS_STABLE,
            .has_axis_range = true,
            .axis_data_min = visible_min,
            .axis_data_max = visible_max,
        };
        char offset_units[DVZ_SCENE_LABEL_SIZE] = {0};
        if (!_scene_units_format(
                axis->units, offset, &offset_context, offset_units, sizeof(offset_units)))
            return false;
        dvz_snprintf(out->offset_label, sizeof(out->offset_label), "+%s", offset_units);

        double residual_min = visible_min - offset;
        double residual_max = visible_max - offset;
        DvzUnitFormatContext residual_context = {
            .mode = DVZ_UNIT_DISPLAY_AXIS_STABLE,
            .has_axis_range = true,
            .axis_data_min = residual_min,
            .axis_data_max = residual_max,
        };
        for (uint32_t i = 0; i < tick_count; i++)
        {
            if (!_scene_units_format(
                    axis->units, ticks[i] - offset, &residual_context, out->tick_labels[i],
                    sizeof(out->tick_labels[i])))
                return false;
        }
    }
    else
    {
        char offset_text[DVZ_SCENE_LABEL_SIZE] = {0};
        _axis_format_compact_number(offset, step, offset_text, sizeof(offset_text));
        dvz_snprintf(out->offset_label, sizeof(out->offset_label), "+%s", offset_text);
        for (uint32_t i = 0; i < tick_count; i++)
            _axis_format_compact_number(
                ticks[i] - offset, step, out->tick_labels[i], sizeof(out->tick_labels[i]));
    }
    out->has_offset_label = true;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _axis_label_plan(
    const DvzAxis* axis, const double* ticks, uint32_t tick_count, double visible_min,
    double visible_max, DvzAxisLabelPlan* out)
{
    if (axis == NULL || ticks == NULL || out == NULL || tick_count > DVZ_SCENE_MAX_AXIS_TICKS)
        return false;
    memset(out, 0, sizeof(*out));
    out->tick_count = tick_count;
    if (tick_count == 0)
        return true;

    double range_min = fmin(visible_min, visible_max);
    double range_max = fmax(visible_min, visible_max);
    if (_axis_label_plan_datetime(axis, ticks, tick_count, range_min, range_max, out))
        return true;
    if (_axis_label_plan_numeric_offset(axis, ticks, tick_count, range_min, range_max, out))
        return true;

    for (uint32_t i = 0; i < tick_count; i++)
    {
        _axis_format_tick_label(
            axis, ticks[i], range_min, range_max, out->tick_labels[i],
            sizeof(out->tick_labels[i]));
    }
    return true;
}
