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

#include "scene/ticks.h"
#include "_macros.h"

#include "_debug.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define INF            1000000000
#define J_MAX          10
#define K_MAX          50
#define Z_MAX          18
#define PRECISION      2
#define TARGET_DENSITY .2
#define SCORE_WEIGHTS  {0.2, 0.25, 0.5, 0.05}
#define CLOSE(x, y)    (fabs((x) - (y)) < EPSILON)



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct Q Q;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct Q
{
    int32_t i;
    double value;
    uint32_t len;
};



/*************************************************************************************************/
/*  Scoring functions                                                                            */
/*************************************************************************************************/

DVZ_INLINE double simplicity(Q q, int32_t j, double lmin, double lmax, double lstep)
{
    double eps = EPSILON;
    int64_t n = q.len;
    int32_t i = q.i + 1;
    int32_t v = 0;
    if ((fmod(lmin, lstep) < eps) ||
        (((fmod(lstep - lmin, lstep)) < eps) && (lmin <= 0) && (lmax >= 0)))
        v = 1;
    else
        v = 0;

    return (n - i) / (n - 1.0) + v - j;
}



DVZ_INLINE double simplicity_max(Q q, int32_t j)
{
    int64_t n = q.len;
    int32_t i = q.i + 1;
    int32_t v = 1;
    return (n - i) / (n - 1.0) + v - j;
}



DVZ_INLINE double coverage(double dmin, double dmax, double lmin, double lmax)
{
    return 1. - 0.5 * (pow(dmax - lmax, 2) + pow(dmin - lmin, 2)) / pow(0.1 * (dmax - dmin), 2);
}



DVZ_INLINE double coverage_max(double dmin, double dmax, double span)
{
    double drange = dmax - dmin;
    if (span > drange)
        return 1. - pow(0.5 * (span - drange), 2) / pow(0.1 * drange, 2);
    else
        return 1.;
}



DVZ_INLINE double density(double k, double m, double dmin, double dmax, double lmin, double lmax)
{
    double r = (k - 1.) / (lmax - lmin);
    double rt = (m - 1.) / (fmax(lmax, dmax) - fmin(lmin, dmin));
    return 2. - fmax(r / rt, rt / r);
}



DVZ_INLINE double density_max(int32_t k, int32_t m)
{
    if (k >= m)
        return 2. - (k - 1.0) / (m - 1.0);
    else
        return 1.;
}



/*************************************************************************************************/
/*  Legibility                                                                                   */
/*************************************************************************************************/

#define BETWEEN(min, max) (ax > (min) && ax < (max) ? 1 : 0)

DVZ_INLINE double leg(DvzTicksFormat format, double x)
{
    double ax = fabs(x);
    switch (format)
    {
    case DVZ_TICKS_FORMAT_DECIMAL:
        return BETWEEN(1e-4, 1e+6);
    case DVZ_TICKS_FORMAT_DECIMAL_FACTORED:
        return .5;
    case DVZ_TICKS_FORMAT_THOUSANDS:
        return .75 * BETWEEN(1e+3, 1e+6);
    case DVZ_TICKS_FORMAT_THOUSANDS_FACTORED:
        return .4 * BETWEEN(1e+3, 1e+6);
    case DVZ_TICKS_FORMAT_MILLIONS:
        return .75 * BETWEEN(1e+6, 1e+9);
    case DVZ_TICKS_FORMAT_MILLIONS_FACTORED:
        return .4 * BETWEEN(1e+6, 1e+9);
    case DVZ_TICKS_FORMAT_SCIENTIFIC:
        return .25;
    case DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED:
        return .3;
    default:
        log_error("unknown format %d", format);
        return 0;
    }
    return 0;
}



DVZ_INLINE double overlap(double d)
{
    if (d >= DIST_MIN)
        return 1;
    else if (d <= 0)
        return -INF;
    else
    {
        ASSERT(d >= 0);
        ASSERT(d < DIST_MIN);
        return 2 - DIST_MIN / d;
    }
}



DVZ_INLINE double estimate_label_length(DvzTicksFormat format, double x)
{
    if (x == 0)
        return 0;

    // TODO: improve these estimates.
    switch (format)
    {
    case DVZ_TICKS_FORMAT_DECIMAL:
        return 1 + log10(fabs(x));
    case DVZ_TICKS_FORMAT_DECIMAL_FACTORED:
        return 3;
    case DVZ_TICKS_FORMAT_THOUSANDS:
        return 4;
    case DVZ_TICKS_FORMAT_THOUSANDS_FACTORED:
        return 3;
    case DVZ_TICKS_FORMAT_MILLIONS:
        return 4;
    case DVZ_TICKS_FORMAT_MILLIONS_FACTORED:
        return 3;
    case DVZ_TICKS_FORMAT_SCIENTIFIC:
        return 5;
    case DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED:
        return 3;
    default:
        log_error("unknown format %d", format);
        return 0;
    }
    return 0;
}



DVZ_INLINE double min_distance_labels(DvzTicks* ticks)
{
    ANN(ticks);
    double size = ticks->range_size;
    ASSERT(size > 0);

    double glyph = ticks->glyph_size;
    ASSERT(glyph > 0);

    double lmin = ticks->lmin;
    double lmax = ticks->lmax;
    double lstep = ticks->lstep;
    if (lmin >= lmax)
        return 0;
    ASSERT(lmax - lmin > 0);

    uint32_t n = tick_count(lmin, lmax, lstep); // number of labels
    ASSERT(n > 0);
    double label_length = 0, x = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        x = lmin + i * lstep;
        label_length += estimate_label_length(ticks->format, x); // number of glyphs per label
    }
    label_length /= n;

    // minimum distance between two labels under the simplifying assumption that all labels have
    // the same size.
    // TODO: make the actual computation instead.
    return (n - 1) * lstep / (lmax - lmin) * size - label_length * glyph;
}



static inline double legibility(DvzTicks* ticks)
{
    double lmin = ticks->lmin;
    double lmax = ticks->lmax;
    double lstep = ticks->lstep;
    uint32_t n = tick_count(lmin, lmax, lstep);
    if (n == 0)
        return -INF;

    ASSERT(n > 0);
    ASSERT(lmin < lmax);
    ASSERT(lstep > 0);

    double f = 0;

    // Format part.
    double x = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        x = lmin + i * lstep;
        ASSERT(x <= lmax + .5 * lstep);
        f += leg(ticks->format, x);
    }
    f = .9 * f / MAX(1, n);
    f += .1;
    // NOTE: need to add 0.1 if all ticks are extended with 0 such that all
    // have the same number of decimals.

    // Overlap part.
    double o = overlap(min_distance_labels(ticks));

    ASSERT(f <= 1.0 + EPSILON);
    ASSERT(o <= 1.0 + EPSILON);

    double out = (f + o) / 2.0;
    if (out < -INF / 10)
        out = -INF;
    return out;
}



/*************************************************************************************************/
/*  Score                                                                                        */
/*************************************************************************************************/

static inline double
score(dvec4 weights, double simplicity, double coverage, double density, double legibility)
{
    double s = weights[0] * simplicity + weights[1] * coverage + //
               weights[2] * density + weights[3] * legibility;
    if (s < -INF / 10)
        s = -INF;
    return s;
}



/*************************************************************************************************/
/*  Algorithm                                                                                    */
/*************************************************************************************************/

// Optimize format for legibility.
static inline void opt_format(DvzTicks* ticks)
{
    double l = -INF, best_l = -INF;
    DvzTicksFormat best_format = DVZ_TICKS_FORMAT_UNDEFINED;
    for (uint32_t f = 1; f < DVZ_TICKS_FORMAT_COUNT; f++)
    {
        ticks->format = (DvzTicksFormat)f;
        l = legibility(ticks);
        if (l > best_l)
        {
            best_format = ticks->format;
            best_l = l;
        }
    }
    if (best_format != DVZ_TICKS_FORMAT_UNDEFINED)
    {
        ticks->format = best_format;
    }
}



/*
k : current number of labels
m : requested number of labels
q : nice number
j : skip, amount among a sequence of nice numbers
z : 10-exponent of the step size
*/
static void wilk_ext(DvzTicks* ticks, int32_t m)
{
    ANN(ticks);

    double dmin = ticks->dmin;
    double dmax = ticks->dmax;
    double range_size = ticks->range_size;
    double glyph_size = ticks->glyph_size;

    log_debug(
        "starting extended Wilkinson algorithm for tick positioning: [%.3f, %.3f]", dmin, dmax);

    ASSERT(dmin <= dmax);
    ASSERT(range_size > 0);
    ASSERT(glyph_size > 0);
    ASSERT(m > 0);
    if (range_size < 10 * glyph_size)
    {
        log_debug("degenerate axes context, return a trivial tick range");
        return;
    }

    double DEFAULT_Q[] = {1, 5, 2, 2.5, 4, 3};
    dvec4 W = SCORE_WEIGHTS; // score weights

    double n = (double)sizeof(DEFAULT_Q) / sizeof(DEFAULT_Q[0]);
    double best_score = -INF;
    Q q = {0};
    q.i = 0;
    q.len = n;
    q.value = DEFAULT_Q[0];
    // q.values = DEFAULT_Q;

    DvzTicks best_ticks = {0};
    best_ticks = *ticks;
    int32_t j = 1;
    int32_t u, k;
    double sm, dm, delta, z, step, cm, min_start, max_start, l, lmin, lmax, lstep, start, s, c, d,
        scr;
    while (j < J_MAX)
    {
        // printf("j %d\n", j);
        for (u = 0; u < n; u++)
        {
            // printf("u %d\n", u);
            q.i = u;
            q.value = DEFAULT_Q[q.i];
            sm = simplicity_max(q, j);

            if (score(W, sm, 1, 1, 1) <= best_score)
            {
                j = INF;
                break;
            }

            k = 2;
            while (k < K_MAX)
            {
                // printf("k %d\n", k);
                dm = density_max(k, m);

                if (score(W, sm, 1, dm, 1) <= best_score)
                    break;

                delta = (dmax - dmin) / (k + 1.) / (j * q.value);
                z = ceil(log10(delta));

                while (z < Z_MAX)
                {
                    // printf("z %f\n", z);
                    ASSERT(j > 0);
                    ASSERT(q.value > 0);
                    step = j * q.value * pow(10., z);
                    ASSERT(step > 0);
                    cm = coverage_max(dmin, dmax, step * (k - 1.));

                    if (score(W, sm, cm, dm, 1) <= best_score)
                        break;

                    min_start = floor(dmax / step) * j - (k - 1.) * j;
                    max_start = ceil(dmin / step) * j;

                    if (min_start > max_start)
                    {
                        z++;
                        break;
                    }

                    for (start = min_start; start <= max_start; start++)
                    {
                        lmin = start * (step / j);
                        lmax = lmin + step * (k - 1.0);
                        lstep = step;

                        ticks->lmin = lmin;
                        ticks->lmax = lmax;
                        ticks->lstep = lstep;

                        s = simplicity(q, j, lmin, lmax, lstep);
                        c = coverage(dmin, dmax, lmin, lmax);
                        d = density(k, m, dmin, dmax, lmin, lmax);

                        if (score(W, s, c, d, 1) <= best_score)
                            continue;

                        // The following optimizes format in-place.
                        opt_format(ticks);
                        l = legibility(ticks);

                        scr = score(W, s, c, d, l);
                        if (scr > best_score)
                        {
                            best_score = scr;
                            // Keep track of the best result so far.
                            best_ticks.lmin = lmin;
                            best_ticks.lmax = lmax;
                            best_ticks.lstep = lstep;

                            // Best format is stored in the ticks struct.
                            best_ticks.format = ticks->format;
                        }
                    }
                    z++;
                }
                k++;
            }
        }
        j++;
    }

    ticks->lmin = best_ticks.lmin;
    ticks->lmax = best_ticks.lmax;
    ticks->lstep = best_ticks.lstep;
    ticks->format = best_ticks.format;
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



// range_size is in the same unit as glyph size, it's the size between dmin
// and dmax (below)
void dvz_ticks_size(DvzTicks* ticks, double range_size, double glyph_size)
{
    ANN(ticks);
    ticks->range_size = range_size;
    ticks->glyph_size = glyph_size;
}



bool dvz_ticks_compute(DvzTicks* ticks, double dmin, double dmax, uint32_t requested_count)
{
    ANN(ticks);
    ticks->dmin = dmin;
    ticks->dmax = dmax;

    // Keep track of the initial parameters.
    double lmin = ticks->lmin;
    double lmax = ticks->lmax;
    double lstep = ticks->lstep;
    DvzTicksFormat format = ticks->format;

    // Run the algorithm.
    int32_t m = (int32_t)requested_count;
    wilk_ext(ticks, m);

    // Determine whether the parameters are different.
    bool has_changed =
        (!CLOSE(lmin, ticks->lmin) ||   //
         !CLOSE(lmax, ticks->lmax) ||   //
         !CLOSE(lstep, ticks->lstep) || //
         (format != ticks->format)      //
        );

    log_info(
        "extended Wilkinson algorithm finished (changed %d): "
        "lmin=%.3f, lmax=%.3f, lstep=%.3f",
        has_changed, lmin, lmax, lstep);

    return has_changed;
}



uint32_t dvz_ticks_range(DvzTicks* ticks, double* lmin, double* lmax, double* lstep)
{
    ANN(ticks);
    ANN(lmin);
    ANN(lmax);
    ANN(lstep);

    *lmin = ticks->lmin;
    *lmax = ticks->lmax;
    *lstep = ticks->lstep;

    return tick_count(*lmin, *lmax, *lstep);
}



DvzTicksFormat dvz_ticks_format(DvzTicks* ticks)
{
    // decimal or scientific notation
    ANN(ticks);
    return ticks->format;
}



// uint32_t dvz_ticks_precision(DvzTicks* ticks)
// {
//     // number of digits after the dot
//     ANN(ticks);
//     return ticks->precision;
// }



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
    log_info(
        "%.6f -> %.6f (step %.6f), format %d", //
        ticks->lmin, ticks->lmax, ticks->lstep, ticks->format);
}



void dvz_ticks_destroy(DvzTicks* ticks)
{
    ANN(ticks);
    FREE(ticks);
}
