/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Ticks                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_macros.h"
#include "scene/ticks.h"

#include "_debug.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MAX_LABEL_LEN 64



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

DVZ_INLINE int32_t rlog10(double value)
{
    double a = fabs(value);
    return a > 1e-14 ? (int32_t)floor(log10(a)) : -INFINITY;
}



DVZ_INLINE double pow10_(int32_t exp) { return pow(10.0, (double)exp); }



/*************************************************************************************************/
/*  Scoring functions                                                                            */
/*************************************************************************************************/

// Choose a "nice" number for the tick step size.
DVZ_INLINE double nice_number(double range, bool round)
{
    double exponent = floor(log10(range));
    double fraction = range / pow(10, exponent);
    double nice_fraction;

    if (round)
    {
        if (fraction < 1.5)
            nice_fraction = 1;
        else if (fraction < 3)
            nice_fraction = 2;
        else if (fraction < 7)
            nice_fraction = 5;
        else
            nice_fraction = 10;
    }
    else
    {
        if (fraction <= 1)
            nice_fraction = 1;
        else if (fraction <= 2)
            nice_fraction = 2;
        else if (fraction <= 5)
            nice_fraction = 5;
        else
            nice_fraction = 10;
    }

    return nice_fraction * pow(10, exponent);
}



DVZ_INLINE uint32_t count_decimal_places(double step)
{
    if (step <= 0.0)
        return 0;

    // HACK: work around precision issues.
    step = round(step * pow10_(14)) / pow10_(14);

    double frac = step - floor(step);
    uint32_t precision = 0;
    while (frac > 1e-10 && precision < 10)
    {
        step *= 10;
        frac = step - floor(step);
        precision++;
    }
    return precision;
}



/*************************************************************************************************/
/*  Ticks functions                                                                              */
/*************************************************************************************************/

DvzTicks* dvz_ticks(int flags)
{
    DvzTicks* ticks = (DvzTicks*)calloc(1, sizeof(DvzTicks));
    ticks->flags = flags;
    return ticks;
}



// range_size is in the same unit as glyph size, it's the size between dmin and dmax (below)
void dvz_ticks_size(DvzTicks* ticks, double range_size, double glyph_size)
{
    ANN(ticks);
    ticks->range_size = range_size;
    ticks->glyph_size = glyph_size;
}



bool dvz_ticks_compute(DvzTicks* ticks, double dmin, double dmax, uint32_t requested_count)
{
    ANN(ticks);
    ASSERT(requested_count > 0);
    log_debug(
        "computing ticks on [%.3f, %.3f] (requesting %d ticks)", dmin, dmax, requested_count);
    if (dmin >= dmax)
    {
        log_error("invalid range [%.3f, %.3f]", dmin, dmax);
        return false;
    }
    ticks->dmin = dmin;
    ticks->dmax = dmax;

    // Keep track of the initial parameters.
    double lmin_orig = ticks->lmin;
    double lmax_orig = ticks->lmax;
    double lstep_orig = ticks->lstep;
    DvzTicksSpec spec_orig = ticks->spec;

    double range = dmax - dmin;
    double raw_step = range / requested_count;

    // Step 1: Find a "nice" step size
    double step = nice_number(raw_step, true);

    // Step 2: Compute nice lmin and lmax
    double lmin = floor(dmin / step) * step;
    double lmax = ceil(dmax / step) * step;

    ticks->lstep = step;
    ticks->lmin = lmin;
    ticks->lmax = lmax;

    // Step 3: Decide formatting
    DvzTicksFormat format = DVZ_TICKS_FORMAT_DECIMAL;

    double almin = fabs(lmin);
    double almax = fabs(lmax);
    double abs_max = fmax(almin, almax);
    double diff = fabs(lmax - lmin);

    int32_t expdiff = rlog10(diff);
    int32_t expmin = rlog10(almin);
    int32_t expmax = rlog10(almax);
    int32_t global_exponent = rlog10(abs_max);

    bool is_factored_exp = false;
    bool is_factored_offset = false;
    int32_t exponent = 0;
    uint32_t precision = 0;
    double offset = 0;

    // To determine if the format should be factored: two cases:
    if (lmin * lmax <= 0)
    {
        // 0 is in the interval: no offset, exponent depends on scale of lmax.
        is_factored_exp = fabs((double)global_exponent) >= 4; // fabs(MAX(expmin, expmax)) >= 4;
        is_factored_offset = false;

        exponent = is_factored_exp ? global_exponent : 0;
        offset = 0;
    }
    else
    {
        // 0 is not in the interval: offset depends on relative scale of diff vs almin, exponent
        // depends on scale of diff.
        is_factored_exp = fabs((double)rlog10(diff)) >= 4;
        is_factored_offset = fabs((double)rlog10(diff / almin)) >= 4;

        offset = is_factored_offset ? .5 * (lmin + lmax) : 0;
        exponent = is_factored_exp ? expdiff : 0;
    }
    format = is_factored_exp || is_factored_offset ? DVZ_TICKS_FORMAT_FACTORED
                                                   : DVZ_TICKS_FORMAT_DECIMAL;

    {
        // // Check if values are far from zero and tightly clustered
        // bool clustered = false;
        // {
        //     // How many digits differ between lmin and lmax?
        //     double diff = fabs(lmax - lmin);
        //     double max_abs = fmax(fabs(lmin), fabs(lmax));

        //     if (diff <= 0 || max_abs <= 0)
        //     {
        //         clustered = false;
        //     }
        //     else
        //     {
        //         int digits_diff = (int)floor(log10(diff));
        //         int digits_common = (int)floor(log10(max_abs)) - digits_diff;

        //         // Consider values clustered if at least 3 common leading digits
        //         if (digits_common >= 3)
        //             clustered = true;
        //     }
        // }

        // // Check exponent clustering (shared magnitude)
        // {
        //     int exp_min = (int)floor(log10(fabs(lmin)));
        //     int exp_max = (int)floor(log10(fabs(lmax)));

        //     if (abs(exp_min - exp_max) <= 1 && abs(exp_min) >= 4)
        //         clustered = true;
        // }

        // if (clustered)
        // {
        //     // Use factored format
        //     // format = DVZ_TICKS_FORMAT_DECIMAL_FACTORED;
        //     format = DVZ_TICKS_FORMAT_FACTORED;

        //     // Decide between decimal and scientific based on tick range
        //     double local_range = fabs(lmax - lmin);
        //     int32_t local_exponent = (int32_t)floor(log10(local_range));
        //     // if (fabs((double)local_exponent) >= 4 ||
        //     //     fabs(step / pow10_((double)local_exponent)) < 1e-3)
        //     //     format = DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED;

        //     // Offset = start of tick range
        //     ticks->spec.offset = .5 * (lmin + lmax);

        //     // Exponent = based on range, not absolute values
        //     ticks->spec.exponent = local_exponent;
        //     // (format == DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED) ? local_exponent : 0;
        // }
        // else
        // {
        //     // Use standard (non-factored) formats
        //     double abs_max = fmax(fabs(lmin), fabs(lmax));
        //     int32_t global_exponent = (int32_t)floor(log10(abs_max));
        //     if (fabs((double)global_exponent) >= 4 || fabs(step) < 1e-3)
        //     {
        //         format = DVZ_TICKS_FORMAT_SCIENTIFIC;
        //         // ticks->spec.exponent = global_exponent;
        //     }
        //     else
        //     {
        //         format = DVZ_TICKS_FORMAT_DECIMAL;
        //         ticks->spec.exponent = 0;
        //     }

        //     ticks->spec.offset = 0;
        // }
    }
    log_debug("found format %d", format);

    // Compute precision based on step size


    if (format == DVZ_TICKS_FORMAT_DECIMAL)
    {
        precision = count_decimal_places(step);
    }
    else if (format == DVZ_TICKS_FORMAT_SCIENTIFIC)
    {
        double scale = pow(10, global_exponent);
        double ratio = step / scale;

        // How many digits needed to express ratio
        precision = ratio >= 1 ? 0 : (uint32_t)(ceil(-log10(ratio)));
    }
    else if (is_factored_exp)
    {
        precision = count_decimal_places(step / pow10_(exponent));
    }
    else if (is_factored_offset)
    {
        precision = count_decimal_places(step);
    }

    log_debug("found precision %d", precision);

    ticks->spec.format = format;
    ticks->spec.offset = offset;
    ticks->spec.exponent = exponent;
    ticks->spec.precision = precision;

    // Determine whether the parameters are different.
    bool has_changed =
        (!CLOSE(lmin_orig, ticks->lmin) ||          //
         !CLOSE(lmax_orig, ticks->lmax) ||          //
         !CLOSE(lstep_orig, ticks->lstep) ||        //
         !_are_spec_equal(&spec_orig, &ticks->spec) //
        );

    // log_info(
    //     "algorithm finished (changed %d): "
    //     "lmin=%.3f, lmax=%.3f, lstep=%.3f",
    //     has_changed, ticks->lmin, ticks->lmax, ticks->lstep);

    return has_changed;
}



void dvz_ticks_range(DvzTicks* ticks, double* lmin, double* lmax, double* lstep)
{
    ANN(ticks);
    ANN(lmin);
    ANN(lmax);
    ANN(lstep);

    *lmin = ticks->lmin;
    *lmax = ticks->lmax;
    *lstep = ticks->lstep;

    // return get_tick_count(*lmin, *lmax, *lstep);
}



DvzTicksSpec dvz_ticks_spec(DvzTicks* ticks)
{
    ANN(ticks);
    return ticks->spec;
}



// return true if the score with that range is significantly bottom than the current score
bool dvz_ticks_dirty(DvzTicks* ticks, double dmin, double dmax)
{
    ANN(ticks);
    // double current_score = dvz_ticks_score(ticks);

    // // Make a copy.
    // DvzTicks other = {0};
    // memcpy(&other, ticks, sizeof(DvzTicks));
    // other.dmin = dmin;
    // other.dmax = dmax;
    // double score = dvz_ticks_score(&other);

    // // TODO: compare current_score and score.
    // ASSERT(current_score);
    // ASSERT(score);


    return false;
}



void dvz_ticks_print(DvzTicks* ticks)
{
    ANN(ticks);
    uint32_t tick_count = get_tick_count(ticks->lmin, ticks->lmax, ticks->lstep);
    if (_is_format_factored(&ticks->spec))
    {
        double offset = ticks->spec.offset;
        int32_t exponent = ticks->spec.exponent;

        printf(
            "Ticks:\n[%.4f, %.4f] => [%.4f, %.4f] E%d + %.4f (%d, step %.6f), format %d\n\n", //
            ticks->dmin, ticks->dmax,                                                         //
            (ticks->lmin - offset) / pow10_(exponent),
            (ticks->lmax - offset) / pow10_(exponent), //
            exponent, offset, tick_count, ticks->lstep, ticks->spec.format);
    }
    else
    {
        printf(
            "Ticks:\n[%.6f, %.6f] => [%.6f, %.6f] (%d, step %.6f), format %d\n\n", //
            ticks->dmin, ticks->dmax, ticks->lmin, ticks->lmax, tick_count, ticks->lstep,
            ticks->spec.format);
    }
}



void dvz_ticks_clear(DvzTicks* ticks)
{
    ANN(ticks);
    ticks->lmin = 0;
    ticks->lmax = 0;
    ticks->lstep = 0;

    memset(&ticks->spec, 0, sizeof(ticks->spec));
}



void dvz_ticks_destroy(DvzTicks* ticks)
{
    ANN(ticks);
    FREE(ticks);
}



/*************************************************************************************************/
/*  Tick label generation                                                                        */
/*************************************************************************************************/

void dvz_ticks_linspace(
    DvzTicksSpec* spec, uint32_t tick_count, double lmin, double lmax, double lstep,
    char** out_labels, double* out_tick_pos)
{
    ASSERT(tick_count > 0);
    ASSERT(lmax > lmin);

    // Step 1: Generate raw ticks.
    for (uint32_t i = 0; i < tick_count; i++)
    {
        double val = lmin + i * lstep;
        out_tick_pos[i] = val;
    }

    // Step 2: Determine offset and exponent (if factored format)
    bool factored = _is_format_factored(spec);

    // Step 3: Format labels
    for (uint32_t i = 0; i < tick_count; i++)
    {
        double val = out_tick_pos[i];
        double display_val = val;

        if (factored)
        {
            display_val = (val - spec->offset) / pow10_(spec->exponent);
        }

        char* label = (char*)calloc(MAX_LABEL_LEN, sizeof(char));
        out_labels[i] = label;

        switch (spec->format)
        {
        case DVZ_TICKS_FORMAT_DECIMAL:
        case DVZ_TICKS_FORMAT_FACTORED:
            snprintf(label, MAX_LABEL_LEN, "%.*f", spec->precision, display_val);
            break;

        case DVZ_TICKS_FORMAT_SCIENTIFIC:
            snprintf(label, MAX_LABEL_LEN, "%.*e", spec->precision, display_val);
            break;

        default:
            snprintf(label, MAX_LABEL_LEN, "%.*f", spec->precision, display_val);
            break;
        }
    }
}
