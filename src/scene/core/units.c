/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene units and datetime formatting                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "units_internal.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TIMESTAMP_US_PER_SECOND 1000000LL



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _unit_value_close(double a, double b)
{
    return fabs(a - b) <= 1e-9 * fmax(1.0, fmax(fabs(a), fabs(b)));
}


static void _unit_format_compact(double value, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0)
        return;
    double rounded = round(value);
    if (_unit_value_close(value, rounded))
    {
        dvz_snprintf(out, out_size, "%.0f", rounded);
        return;
    }
    if (fabs(value) >= 10.0)
        dvz_snprintf(out, out_size, "%.1f", value);
    else
        dvz_snprintf(out, out_size, "%.2f", value);

    char* dot = NULL;
    for (char* p = out; *p != '\0'; p++)
    {
        if (*p == '.')
            dot = p;
    }
    if (dot == NULL)
        return;
    char* end = out;
    while (*end != '\0')
        end++;
    while (end > dot + 1 && end[-1] == '0')
    {
        end--;
        *end = '\0';
    }
    if (end > dot && end[-1] == '.')
        end[-1] = '\0';
}


static void _unit_ladder_sort(DvzUnitLadder* ladder)
{
    ANN(ladder);
    for (uint32_t i = 1; i < ladder->entry_count; i++)
    {
        DvzUnitLadderEntry entry = ladder->entries[i];
        uint32_t j = i;
        while (j > 0 && ladder->entries[j - 1].factor > entry.factor)
        {
            ladder->entries[j] = ladder->entries[j - 1];
            j--;
        }
        ladder->entries[j] = entry;
    }
}


static int _unit_ladder_add_entry(DvzUnitLadder* ladder, double factor, const char* label)
{
    if (ladder == NULL || !isfinite(factor) || factor <= 0.0 || label == NULL ||
        label[0] == '\0')
        return -1;
    for (uint32_t i = 0; i < ladder->entry_count; i++)
    {
        if (_unit_value_close(ladder->entries[i].factor, factor) ||
            strcmp(ladder->entries[i].label, label) == 0)
        {
            log_error("duplicate unit ladder entry");
            return -1;
        }
    }
    if (ladder->entry_count >= DVZ_SCENE_MAX_UNIT_LADDER_ENTRIES)
    {
        log_error("unit ladder capacity exceeded");
        return -1;
    }
    DvzUnitLadderEntry* entry = &ladder->entries[ladder->entry_count++];
    entry->factor = factor;
    dvz_strlcpy(entry->label, label, sizeof(entry->label));
    _unit_ladder_sort(ladder);
    return 0;
}


static void _unit_ladder_fill_builtin(DvzUnitLadder* ladder, DvzUnitLadderBuiltin builtin)
{
    ANN(ladder);
    ladder->entry_count = 0;
    ladder->builtin = true;
    ladder->builtin_kind = builtin;
    switch (builtin)
    {
    case DVZ_UNIT_LADDER_METRIC_LENGTH:
        dvz_strlcpy(ladder->canonical_unit, "m", sizeof(ladder->canonical_unit));
        (void)_unit_ladder_add_entry(ladder, 1e-9, "nm");
        (void)_unit_ladder_add_entry(ladder, 1e-6, "um");
        (void)_unit_ladder_add_entry(ladder, 1e-3, "mm");
        (void)_unit_ladder_add_entry(ladder, 1.0, "m");
        (void)_unit_ladder_add_entry(ladder, 1e3, "km");
        break;
    case DVZ_UNIT_LADDER_DURATION:
        dvz_strlcpy(ladder->canonical_unit, "s", sizeof(ladder->canonical_unit));
        (void)_unit_ladder_add_entry(ladder, 1e-9, "ns");
        (void)_unit_ladder_add_entry(ladder, 1e-6, "us");
        (void)_unit_ladder_add_entry(ladder, 1e-3, "ms");
        (void)_unit_ladder_add_entry(ladder, 1.0, "s");
        (void)_unit_ladder_add_entry(ladder, 60.0, "min");
        (void)_unit_ladder_add_entry(ladder, 3600.0, "h");
        break;
    case DVZ_UNIT_LADDER_RAW:
    default:
        dvz_strlcpy(ladder->canonical_unit, "", sizeof(ladder->canonical_unit));
        (void)_unit_ladder_add_entry(ladder, 1.0, "");
        break;
    }
}


static DvzUnitLadder* _scene_unit_ladder_alloc(DvzScene* scene)
{
    if (scene == NULL || scene->unit_ladder_count >= DVZ_SCENE_MAX_UNIT_LADDERS)
        return NULL;
    DvzUnitLadder* ladder = &scene->unit_ladders[scene->unit_ladder_count++];
    memset(ladder, 0, sizeof(*ladder));
    ladder->scene = scene;
    ladder->active = true;
    return ladder;
}


static DvzUnits* _scene_units_alloc(DvzScene* scene)
{
    if (scene == NULL || scene->units_count >= DVZ_SCENE_MAX_UNITS)
        return NULL;
    DvzUnits* units = &scene->units[scene->units_count++];
    memset(units, 0, sizeof(*units));
    units->scene = scene;
    units->active = true;
    units->data_to_canonical = 1.0;
    units->display_mode = DVZ_UNIT_DISPLAY_AUTO;
    return units;
}


static DvzDateTimeFormat* _scene_datetime_format_alloc(DvzScene* scene)
{
    if (scene == NULL || scene->datetime_format_count >= DVZ_SCENE_MAX_DATETIME_FORMATS)
        return NULL;
    DvzDateTimeFormat* format = &scene->datetime_formats[scene->datetime_format_count++];
    memset(format, 0, sizeof(*format));
    format->scene = scene;
    format->active = true;
    dvz_strlcpy(format->timezone, "UTC", sizeof(format->timezone));
    return format;
}


static const DvzUnitLadderEntry*
_unit_ladder_entry_by_label(const DvzUnitLadder* ladder, const char* label)
{
    if (ladder == NULL || label == NULL)
        return NULL;
    for (uint32_t i = 0; i < ladder->entry_count; i++)
    {
        if (strcmp(ladder->entries[i].label, label) == 0)
            return &ladder->entries[i];
    }
    return NULL;
}


static const DvzUnitLadderEntry* _unit_choose_entry(
    const DvzUnits* units, double canonical_value, const DvzUnitFormatContext* context)
{
    ANN(units);
    DvzUnitLadder* ladder = units->ladder;
    if (ladder == NULL || ladder->entry_count == 0)
        return NULL;

    DvzUnitDisplayMode mode = context != NULL ? context->mode : units->display_mode;
    if (mode == DVZ_UNIT_DISPLAY_FIXED)
    {
        const DvzUnitLadderEntry* fixed =
            _unit_ladder_entry_by_label(ladder, units->fixed_label);
        if (fixed != NULL)
            return fixed;
    }

    double reference = fabs(canonical_value);
    if (mode == DVZ_UNIT_DISPLAY_AXIS_STABLE && context != NULL && context->has_axis_range)
    {
        double a = fabs(context->axis_data_min * units->data_to_canonical);
        double b = fabs(context->axis_data_max * units->data_to_canonical);
        reference = fmax(a, b);
    }
    if (!isfinite(reference) || reference <= 0.0)
        reference = ladder->entries[0].factor;

    const DvzUnitLadderEntry* chosen = &ladder->entries[0];
    for (uint32_t i = 0; i < ladder->entry_count; i++)
    {
        if (reference >= ladder->entries[i].factor)
            chosen = &ladder->entries[i];
    }
    return chosen;
}


static bool _datetime_gmtime(DvzTimestamp timestamp, struct tm* out, int32_t* out_usec)
{
    ANN(out);
    ANN(out_usec);
    DvzTimestamp seconds = timestamp / DVZ_TIMESTAMP_US_PER_SECOND;
    DvzTimestamp usec = timestamp % DVZ_TIMESTAMP_US_PER_SECOND;
    if (usec < 0)
    {
        seconds--;
        usec += DVZ_TIMESTAMP_US_PER_SECOND;
    }
    time_t tt = (time_t)seconds;
#if defined(_WIN32)
    if (gmtime_s(out, &tt) != 0)
        return false;
#else
    if (gmtime_r(&tt, out) == NULL)
        return false;
#endif
    *out_usec = (int32_t)usec;
    return true;
}


static void _datetime_builtin_rules(DvzDateTimeFormat* format, DvzDateTimeBuiltin builtin)
{
    ANN(format);
    format->builtin = true;
    format->builtin_kind = builtin;
    dvz_strlcpy(format->timezone, "UTC", sizeof(format->timezone));
    for (uint32_t i = 0; i <= DVZ_TIME_INTERVAL_YEAR; i++)
        format->rules[i][0] = '\0';

    if (builtin == DVZ_DATETIME_FORMAT_ISO_UTC)
    {
        dvz_strlcpy(
            format->rules[DVZ_TIME_INTERVAL_MICROSECOND], "%Y-%m-%dT%H:%M:%S.ffffffZ",
            sizeof(format->rules[0]));
        dvz_strlcpy(
            format->rules[DVZ_TIME_INTERVAL_SECOND], "%Y-%m-%dT%H:%M:%SZ",
            sizeof(format->rules[0]));
        dvz_strlcpy(
            format->rules[DVZ_TIME_INTERVAL_DAY], "%Y-%m-%d", sizeof(format->rules[0]));
        dvz_strlcpy(format->rules[DVZ_TIME_INTERVAL_YEAR], "%Y", sizeof(format->rules[0]));
        return;
    }

    dvz_strlcpy(
        format->rules[DVZ_TIME_INTERVAL_MICROSECOND], "%H:%M:%S.ffffff",
        sizeof(format->rules[0]));
    dvz_strlcpy(format->rules[DVZ_TIME_INTERVAL_SECOND], "%H:%M:%S", sizeof(format->rules[0]));
    dvz_strlcpy(format->rules[DVZ_TIME_INTERVAL_MINUTE], "%H:%M", sizeof(format->rules[0]));
    dvz_strlcpy(format->rules[DVZ_TIME_INTERVAL_HOUR], "%H:%M", sizeof(format->rules[0]));
    dvz_strlcpy(format->rules[DVZ_TIME_INTERVAL_DAY], "%b %d", sizeof(format->rules[0]));
    dvz_strlcpy(format->rules[DVZ_TIME_INTERVAL_MONTH], "%Y-%m", sizeof(format->rules[0]));
    dvz_strlcpy(format->rules[DVZ_TIME_INTERVAL_YEAR], "%Y", sizeof(format->rules[0]));
}


static const char* _datetime_rule_for_interval(
    const DvzDateTimeFormat* format, DvzTimeInterval interval)
{
    if (format == NULL)
        return "";
    for (int i = (int)interval; i >= 0; i--)
    {
        if (format->rules[i][0] != '\0')
            return format->rules[i];
    }
    for (uint32_t i = (uint32_t)interval + 1; i <= DVZ_TIME_INTERVAL_YEAR; i++)
    {
        if (format->rules[i][0] != '\0')
            return format->rules[i];
    }
    return "%Y-%m-%dT%H:%M:%SZ";
}


static void _datetime_format_fractional(
    const char* rule, const struct tm* tm, int32_t usec, char* out, size_t out_size)
{
    ANN(rule);
    ANN(tm);
    ANN(out);
    if (out_size == 0)
        return;
    const char* marker = strstr(rule, "ffffff");
    if (marker == NULL)
    {
        if (strftime(out, out_size, rule, tm) == 0)
            out[0] = '\0';
        return;
    }

    char patched[64] = {0};
    size_t prefix = (size_t)(marker - rule);
    if (prefix >= sizeof(patched))
        prefix = sizeof(patched) - 1;
    memcpy(patched, rule, prefix);
    char fraction[16] = {0};
    dvz_snprintf(fraction, sizeof(fraction), "%06d", usec);
    dvz_strlcpy(patched + prefix, fraction, sizeof(patched) - prefix);
    dvz_strlcpy(
        patched + strlen(patched), marker + 6, sizeof(patched) - strlen(patched));
    if (strftime(out, out_size, patched, tm) == 0)
        out[0] = '\0';
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

DvzUnitLadder* dvz_unit_ladder_builtin(DvzScene* scene, DvzUnitLadderBuiltin builtin)
{
    if (scene == NULL)
        return NULL;
    for (uint32_t i = 0; i < scene->unit_ladder_count; i++)
    {
        DvzUnitLadder* ladder = &scene->unit_ladders[i];
        if (ladder->active && ladder->builtin && ladder->builtin_kind == builtin)
            return ladder;
    }
    DvzUnitLadder* ladder = _scene_unit_ladder_alloc(scene);
    if (ladder == NULL)
        return NULL;
    _unit_ladder_fill_builtin(ladder, builtin);
    return ladder;
}


DvzUnitLadder* dvz_unit_ladder_create(DvzScene* scene, const char* canonical_unit)
{
    DvzUnitLadder* ladder = _scene_unit_ladder_alloc(scene);
    if (ladder == NULL)
        return NULL;
    ladder->builtin = false;
    dvz_strlcpy(
        ladder->canonical_unit, canonical_unit != NULL ? canonical_unit : "",
        sizeof(ladder->canonical_unit));
    return ladder;
}


int dvz_unit_ladder_add(DvzUnitLadder* ladder, double factor, const char* label)
{
    if (ladder == NULL || ladder->builtin)
        return -1;
    return _unit_ladder_add_entry(ladder, factor, label);
}


void dvz_unit_ladder_clear(DvzUnitLadder* ladder)
{
    if (ladder == NULL || ladder->builtin)
        return;
    ladder->entry_count = 0;
}


DvzUnits* dvz_units_create(DvzScene* scene)
{
    return _scene_units_alloc(scene);
}


DvzUnits* dvz_units_builtin(
    DvzScene* scene, DvzUnitLadderBuiltin builtin, double data_to_canonical)
{
    DvzUnitLadder* ladder = dvz_unit_ladder_builtin(scene, builtin);
    if (ladder == NULL)
        return NULL;
    DvzUnits* units = dvz_units_create(scene);
    if (units == NULL)
        return NULL;
    if (dvz_units_data_to_canonical(units, data_to_canonical) != 0 ||
        dvz_units_ladder(units, ladder) != 0)
        return NULL;
    return units;
}


int dvz_units_data_to_canonical(DvzUnits* units, double factor)
{
    if (units == NULL || !isfinite(factor) || factor <= 0.0)
        return -1;
    units->data_to_canonical = factor;
    return 0;
}


int dvz_units_ladder(DvzUnits* units, DvzUnitLadder* ladder)
{
    if (units == NULL || ladder == NULL || !ladder->active || ladder->entry_count == 0)
        return -1;
    units->ladder = ladder;
    return 0;
}


int dvz_units_display_mode(DvzUnits* units, DvzUnitDisplayMode mode)
{
    if (units == NULL || mode < DVZ_UNIT_DISPLAY_AUTO || mode > DVZ_UNIT_DISPLAY_FIXED)
        return -1;
    units->display_mode = mode;
    return 0;
}


int dvz_units_fixed_label(DvzUnits* units, const char* label)
{
    if (units == NULL || label == NULL || units->ladder == NULL ||
        _unit_ladder_entry_by_label(units->ladder, label) == NULL)
        return -1;
    dvz_strlcpy(units->fixed_label, label, sizeof(units->fixed_label));
    units->display_mode = DVZ_UNIT_DISPLAY_FIXED;
    return 0;
}


DvzDateTimeFormat* dvz_datetime_format_builtin(
    DvzScene* scene, DvzDateTimeBuiltin builtin)
{
    if (scene == NULL)
        return NULL;
    for (uint32_t i = 0; i < scene->datetime_format_count; i++)
    {
        DvzDateTimeFormat* format = &scene->datetime_formats[i];
        if (format->active && format->builtin && format->builtin_kind == builtin)
            return format;
    }
    DvzDateTimeFormat* format = _scene_datetime_format_alloc(scene);
    if (format == NULL)
        return NULL;
    _datetime_builtin_rules(format, builtin);
    return format;
}


DvzDateTimeFormat* dvz_datetime_format_create(DvzScene* scene)
{
    DvzDateTimeFormat* format = _scene_datetime_format_alloc(scene);
    if (format == NULL)
        return NULL;
    _datetime_builtin_rules(format, DVZ_DATETIME_FORMAT_CONCISE_UTC);
    format->builtin = false;
    return format;
}


int dvz_datetime_format_timezone(DvzDateTimeFormat* format, const char* timezone)
{
    if (format == NULL || timezone == NULL || strcmp(timezone, "UTC") != 0)
    {
        log_error("datetime axes support only UTC in v0.4");
        return -1;
    }
    dvz_strlcpy(format->timezone, "UTC", sizeof(format->timezone));
    return 0;
}


int dvz_datetime_format_rule(
    DvzDateTimeFormat* format, DvzTimeInterval interval, const char* strftime_format)
{
    if (format == NULL || interval < DVZ_TIME_INTERVAL_NANOSECOND ||
        interval > DVZ_TIME_INTERVAL_YEAR || strftime_format == NULL)
        return -1;
    dvz_strlcpy(format->rules[interval], strftime_format, sizeof(format->rules[interval]));
    return 0;
}



/*************************************************************************************************/
/*  Internal API                                                                                 */
/*************************************************************************************************/

bool _scene_units_format(
    const DvzUnits* units, double data_value, const DvzUnitFormatContext* context, char* out,
    size_t out_size)
{
    if (out == NULL || out_size == 0)
        return false;
    if (units == NULL || units->ladder == NULL || !isfinite(data_value))
    {
        dvz_snprintf(out, out_size, "%.6g", data_value);
        return units != NULL && isfinite(data_value);
    }

    double canonical = data_value * units->data_to_canonical;
    const DvzUnitLadderEntry* entry = _unit_choose_entry(units, canonical, context);
    if (entry == NULL || !isfinite(entry->factor) || entry->factor <= 0.0)
        return false;

    char value_str[32] = {0};
    _unit_format_compact(canonical / entry->factor, value_str, sizeof(value_str));
    if (entry->label[0] != '\0')
        dvz_snprintf(out, out_size, "%s %s", value_str, entry->label);
    else
        dvz_snprintf(out, out_size, "%s", value_str);
    return true;
}


bool _scene_datetime_format(
    const DvzDateTimeFormat* format, DvzTimestamp timestamp, DvzTimeInterval interval, char* out,
    size_t out_size)
{
    if (out == NULL || out_size == 0)
        return false;
    if (format == NULL)
        return false;
    struct tm tm = {0};
    int32_t usec = 0;
    if (!_datetime_gmtime(timestamp, &tm, &usec))
        return false;
    const char* rule = _datetime_rule_for_interval(format, interval);
    _datetime_format_fractional(rule, &tm, usec, out, out_size);
    return out[0] != '\0';
}


DvzTimestamp _scene_datetime_data_to_timestamp(const DvzAxis* axis, double value)
{
    if (axis == NULL || !axis->datetime_range_set ||
        fabs(axis->datetime_data1 - axis->datetime_data0) <= 1e-12)
        return 0;
    double t = (value - axis->datetime_data0) / (axis->datetime_data1 - axis->datetime_data0);
    double ts = (double)axis->datetime_t0 +
                t * (double)(axis->datetime_t1 - axis->datetime_t0);
    if (!isfinite(ts))
        return 0;
    return (DvzTimestamp)llround(ts);
}


double _scene_datetime_timestamp_to_data(const DvzAxis* axis, DvzTimestamp timestamp)
{
    if (axis == NULL || !axis->datetime_range_set ||
        axis->datetime_t1 == axis->datetime_t0)
        return 0.0;
    double t = ((double)timestamp - (double)axis->datetime_t0) /
               (double)(axis->datetime_t1 - axis->datetime_t0);
    return axis->datetime_data0 + t * (axis->datetime_data1 - axis->datetime_data0);
}
