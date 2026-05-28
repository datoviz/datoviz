/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene scale tick helpers                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "_compat.h"
#include "_scale_ticks.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const double SCALE_TICK_FACTORS[] = {1.0, 2.0, 5.0};

typedef struct SiPrefix
{
    int exponent;
    const char* prefix;
} SiPrefix;



static const SiPrefix SI_PREFIXES[] = {
    {-9, "n"},
    {-6, "u"},
    {-3, "m"},
    {-2, "c"},
    {0, ""},
    {3, "k"},
    {6, "M"},
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether two floating-point values are close enough for display rounding.
 *
 * @param a first value
 * @param b second value
 * @return whether the values are close
 */
static bool _scale_value_close(double a, double b)
{
    return fabs(a - b) <= 1e-9 * fmax(1.0, fmax(fabs(a), fabs(b)));
}



/**
 * Format a short decimal value without unnecessary trailing zeroes.
 *
 * @param value the value
 * @param out output string
 * @param out_size output string size
 */
static void _scale_format_compact(double value, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0)
        return;
    double rounded = round(value);
    if (_scale_value_close(value, rounded))
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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a nice step size with the standard 1/2/5 ladder.
 *
 * @param value positive input value
 * @param round_to_nearest whether to round to the nearest nice value
 * @return nice value, or zero for invalid input
 */
double _scene_nice_number(double value, bool round_to_nearest)
{
    if (!isfinite(value) || value <= 0.0)
        return 0.0;
    double exponent = floor(log10(value));
    double fraction = value / pow(10.0, exponent);
    double nice_fraction = 10.0;
    if (round_to_nearest)
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
 * Choose an adaptive scale-bar length from pixel constraints.
 *
 * @param units_per_px physical units represented by one screen pixel
 * @param target_px preferred screen length in pixels
 * @param min_px minimum acceptable screen length in pixels
 * @param max_px maximum acceptable screen length in pixels
 * @param out_units selected semantic length in physical units
 * @param out_px selected screen length in pixels
 * @return whether a valid length was selected
 */
bool _scene_scalebar_choose_length(
    double units_per_px, float target_px, float min_px, float max_px, double* out_units,
    float* out_px)
{
    if (out_units == NULL || out_px == NULL || !isfinite(units_per_px) || units_per_px <= 0.0)
        return false;
    if (!isfinite(target_px) || target_px <= 0.0f)
        target_px = 120.0f;
    if (!isfinite(min_px) || min_px <= 0.0f)
        min_px = 0.5f * target_px;
    if (!isfinite(max_px) || max_px <= 0.0f)
        max_px = 1.5f * target_px;
    if (max_px < min_px)
    {
        float tmp = max_px;
        max_px = min_px;
        min_px = tmp;
    }

    double best_units = 0.0;
    float best_px = 0.0f;
    double best_score = INFINITY;
    double fallback_units = 0.0;
    float fallback_px = 0.0f;
    double fallback_score = INFINITY;

    for (int exponent = -18; exponent <= 18; exponent++)
    {
        double power = pow(10.0, (double)exponent);
        for (uint32_t i = 0; i < 3; i++)
        {
            double units = SCALE_TICK_FACTORS[i] * power;
            double px = units / units_per_px;
            if (!isfinite(px) || px <= 0.0)
                continue;
            double score = fabs(px - (double)target_px);
            if (score < fallback_score)
            {
                fallback_score = score;
                fallback_units = units;
                fallback_px = (float)px;
            }
            if (px < (double)min_px || px > (double)max_px)
                continue;
            if (score < best_score)
            {
                best_score = score;
                best_units = units;
                best_px = (float)px;
            }
        }
    }

    if (best_units <= 0.0)
    {
        best_units = fallback_units;
        best_px = fallback_px;
    }
    if (best_units <= 0.0)
        return false;
    *out_units = best_units;
    *out_px = best_px;
    return true;
}



/**
 * Format a value with an ASCII SI prefix.
 *
 * @param value value in base units
 * @param unit base unit suffix
 * @param out output string
 * @param out_size output string size
 */
void _scene_format_si_value(double value, const char* unit, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0)
        return;
    if (!isfinite(value))
    {
        dvz_snprintf(out, out_size, "nan");
        return;
    }
    if (unit == NULL)
        unit = "";

    double abs_value = fabs(value);
    const SiPrefix* chosen = &SI_PREFIXES[4];
    for (uint32_t i = 0; i < sizeof(SI_PREFIXES) / sizeof(SI_PREFIXES[0]); i++)
    {
        double factor = pow(10.0, (double)SI_PREFIXES[i].exponent);
        double scaled = abs_value / factor;
        if (scaled >= 1.0)
            chosen = &SI_PREFIXES[i];
    }
    double scaled_value = value / pow(10.0, (double)chosen->exponent);
    char value_str[32] = {0};
    _scale_format_compact(scaled_value, value_str, sizeof(value_str));
    if (unit[0] != '\0')
        dvz_snprintf(out, out_size, "%s %s%s", value_str, chosen->prefix, unit);
    else
        dvz_snprintf(out, out_size, "%s", value_str);
}
