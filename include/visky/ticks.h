/*
C implementation of the Extended Wilkinson’s Algorithm, see

http://vis.stanford.edu/papers/tick-labels
http://vis.stanford.edu/files/2010-TickLabels-InfoVis.pdf

Implementation adapted from Adam Lucke's at

https://github.com/quantenschaum/ctplot/blob/master/ctplot/ticks.py

*/

#ifndef VKL_TICKS_HEADER
#define VKL_TICKS_HEADER

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/visky/common.h"



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

#define INF                 1000000000
#define J_MAX               10
#define K_MAX               50
#define Z_MAX               18
#define PRECISION_MAX       9
#define DIST_MIN            50
#define MAX_GLYPHS_PER_TICK 24
#define MAX_LABELS          256
#define TARGET_DENSITY      .2



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Tick format type.
typedef enum
{
    VKL_TICK_FORMAT_UNDEFINED,
    VKL_TICK_FORMAT_DECIMAL,
    VKL_TICK_FORMAT_SCIENTIFIC,
} VklTickFormat;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklAxesContext VklAxesContext;
typedef struct VklAxesTicks VklAxesTicks;
typedef struct Q Q;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklAxesContext
{
    VklAxisCoord coord;
    float size_viewport; // along the current dimension
    float size_glyph;    // either width or height
    float scale_orig;    // scale
    uint32_t extensions; // number of extensions on each side (typically 1)
};



struct VklAxesTicks
{
    double dmin, dmax;           // range values
    double lmin, lmax, lstep;    // tick range  and interval
    double lmin_orig, lmax_orig; // extended range
    uint32_t value_count;        // final number of labels
    uint32_t value_count_req;    // number of values requested
    VklTickFormat format;        // decimal or scientific notation
    uint32_t precision;          // number of digits after the dot
    double* values;              // from lmin to lmax by lstep
    char* labels;                // hold all tick labels
};



struct Q
{
    int32_t i;
    double value;

    uint32_t len;
    double* values;
};



/*************************************************************************************************/
/*  Scoring functions                                                                            */
/*************************************************************************************************/

VKY_INLINE uint32_t tick_count(double lmin, double lmax, double lstep)
{
    return floor(1 + (lmax - lmin) / lstep);
}



VKY_INLINE double coverage(double dmin, double dmax, double lmin, double lmax)
{
    return 1. - 0.5 * (pow(dmax - lmax, 2) + pow(dmin - lmin, 2)) / pow(0.1 * (dmax - dmin), 2);
}



VKY_INLINE double coverage_max(double dmin, double dmax, double span)
{
    double drange = dmax - dmin;
    if (span > drange)
        return 1. - pow(0.5 * (span - drange), 2) / pow(0.1 * drange, 2);
    else
        return 1.;
}



VKY_INLINE double density(double k, double m, double dmin, double dmax, double lmin, double lmax)
{
    double r = (k - 1.) / (lmax - lmin);
    double rt = (m - 1.) / (fmax(lmax, dmax) - fmin(lmin, dmin));
    return 2. - fmax(r / rt, rt / r);
}



VKY_INLINE double density_max(int32_t k, int32_t m)
{
    if (k >= m)
        return 2. - (k - 1.0) / (m - 1.0);
    else
        return 1.;
}



VKY_INLINE double simplicity(Q q, int32_t j, double lmin, double lmax, double lstep)
{
    double eps = 1e-10;
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



VKY_INLINE double simplicity_max(Q q, int32_t j)
{
    int64_t n = q.len;
    int32_t i = q.i + 1;
    int32_t v = 1;
    return (n - i) / (n - 1.0) + v - j;
}



VKY_INLINE double dist_overlap(double d)
{
    if (d >= DIST_MIN)
        return 1;
    else if (d == 0)
        return -INF;
    else
    {
        ASSERT(d >= 0);
        ASSERT(d < DIST_MIN);
        return 2 - DIST_MIN / d;
    }
}



/*************************************************************************************************/
/*  Format                                                                                       */
/*************************************************************************************************/

VKY_INLINE double leg(VklTickFormat format, uint32_t precision, double x)
{
    double ax = fabs(x);
    double l = 0;
    switch (format)
    {
    case VKL_TICK_FORMAT_DECIMAL:
        l = ax > 1e-4 && ax < 1e6 ? 1 : 0;
        break;

    case VKL_TICK_FORMAT_SCIENTIFIC:
        l = .25;
        break;

    default:
        l = 0;
        break;
    }

    return l;
}



VKY_INLINE double min_distance_labels(VklAxesTicks* ticks, VklAxesContext* ctx)
{
    // NOTE: the context must have labels allocated/computed in order for the overlap to be
    // computed.
    // log_info("overlap %.1f %.1f %.1f", lmin, lmax, lstep);

    double d = 0;       // distance between label i and i+1
    double min_d = INF; //
    // double label_overlap = 0; //
    double size = ctx->size_viewport;
    ASSERT(size > 0);
    double glyph = ctx->size_glyph;
    ASSERT(glyph > 0);
    ASSERT(ticks->value_count > 0);

    uint32_t n0 = 1, n1 = 1;
    ASSERT(ticks->labels != NULL);
    ASSERT(strlen(ticks->labels) > 0);

    uint32_t n = ticks->value_count;
    double lmin = ticks->lmin_orig;
    double lmax = ticks->lmax_orig;
    double lstep = ticks->lstep;
    // double x = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        if (i == n - 1)
            break;
        // x = lmin + i * lstep;
        // ASSERT(x <= lmax);

        // NOTE: the size that takes each label on the current coordinate is:
        // - X axis: number of characters in the label times the glyph width
        // - Y axis: always 1 times the glyph height
        if (ctx->coord == VKL_AXES_COORD_X)
        {
            n0 = strlen(&ticks->labels[i * MAX_GLYPHS_PER_TICK]);
            n1 = strlen(&ticks->labels[(i + 1) * MAX_GLYPHS_PER_TICK]);
            ASSERT(n0 > 0);
            ASSERT(n1 > 0);
        }
        // Compute the distance between the current label and the next.
        d = MAX(0, lstep / (lmax - lmin) * size - glyph * (n0 + n1));

        // if (context.coord == 0 && d == 0)
        //     log_info(
        //         "%.1f %.1f dist %d n0=%d %s, n1=%d %s, %.6f", //
        //         context.size_viewport, context.size_glyph,    //
        //         i, n0, &context.labels[i * MAX_GLYPHS_PER_TICK], n1,
        //         &context.labels[(i + 1) * MAX_GLYPHS_PER_TICK], d);

        // // Compute the overlap for the current label.
        // label_overlap = dist_overlap(d);
        // ASSERT(label_overlap <= 1);

        // Compute the minimum overlap between two successive labels.
        if (d < min_d)
            min_d = d;
    }

    // Compute the overlap for the current label.
    // label_overlap = dist_overlap(d);
    // ASSERT(label_overlap <= 1);

    // log_debug("overlap %f %f %f, %f", lmin, lmax, lstep, min_overlap);
    return min_d;
}



VKY_INLINE void _get_tick_format(VklTickFormat format, uint32_t precision, char* fmt)
{
    uint32_t offset = 4;
    strcpy(fmt, "%s%.XF"); // [2] = precision, [3] = f or e
    snprintf(&fmt[offset], 4, "%d", precision);
    switch (format)
    {
    case VKL_TICK_FORMAT_DECIMAL:
        fmt[offset + 1] = 'f';
        break;
    case VKL_TICK_FORMAT_SCIENTIFIC:
        fmt[offset + 1] = 'e';
        break;
    default:
        log_error("unknown tick format %d", format);
        break;
    }
}



VKY_INLINE void _tick_label(double x, char* tick_format, char* out)
{
    if (x == 0)
    {
        snprintf(out, 2, "0");
        return;
    }
    char sign[2] = {0};
    sign[0] = x < 0 ? '-' : '+';
    snprintf(out, MAX_GLYPHS_PER_TICK, tick_format, sign, fabs(x));
    ASSERT(strlen(out) < MAX_GLYPHS_PER_TICK);
}



static void make_labels(VklAxesTicks* ticks, VklAxesContext* ctx, bool extended)
{
    ASSERT(ticks->labels != NULL);
    char tick_format[12] = {0};
    _get_tick_format(ticks->format, ticks->precision, tick_format);

    // double diff = ticks->lmax - ticks->lmin;
    double x0 = ticks->lmin - extended * ctx->extensions * ticks->lstep;
    double x = x0;
    for (uint32_t i = 0; i < ticks->value_count; i++)
    {
        x = x0 + i * ticks->lstep;
        ticks->values[i] = x;
        _tick_label(x, tick_format, &ticks->labels[i * MAX_GLYPHS_PER_TICK]);
    }
}



// Return whether there are duplicate labels.
static bool duplicate_labels(VklAxesTicks* ticks, VklAxesContext* ctx)
{
    uint32_t n = ticks->value_count;
    char* s0 = NULL;
    char* s1 = NULL;
    for (uint32_t i = 0; i < n - 1; i++)
    {
        s0 = &ticks->labels[i * MAX_GLYPHS_PER_TICK];
        s1 = &ticks->labels[(i + 1) * MAX_GLYPHS_PER_TICK];
        if (memcmp(s0, s1, strlen(s0)) == 0)
        {
            return true;
        }
    }
    return false;
}



static double legibility(VklAxesTicks* ticks, VklAxesContext* ctx)
{
    uint32_t n = ticks->value_count;
    double lmin = ticks->lmin;
    double lmax = ticks->lmax;
    double lstep = ticks->lstep;

    ASSERT(lmin < lmax);
    ASSERT(lstep > 0);

    double f = 0;

    // Format part.
    double x = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        x = lmin + i * lstep;
        ASSERT(x <= lmax + .5 * lstep);
        f += leg(ticks->format, ticks->precision, x);
    }
    f = .9 * f / MAX(1, n); // TODO: 0-extended?

    // Compute the labels.
    make_labels(ticks, ctx, false);

    // Overlap part.
    ticks->lmin_orig = ticks->lmin;
    ticks->lmax_orig = ticks->lmax;
    double o = dist_overlap(min_distance_labels(ticks, ctx));

    // Duplicates part.
    double d = duplicate_labels(ticks, ctx) ? -INF : 1;

    ASSERT(f <= 1);
    ASSERT(o <= 1);
    ASSERT(d <= 1);

    double out = (f + o + d) / 3.0;
    if (out < -INF / 10)
        out = -INF;
    return out;
}



/*************************************************************************************************/
/*  Algorithm                                                                                    */
/*************************************************************************************************/

static double
score(dvec4 weights, double simplicity, double coverage, double density, double legibility)
{
    double s = weights[0] * simplicity + weights[1] * coverage + //
               weights[2] * density + weights[3] * legibility;
    if (s < -INF / 10)
        s = -INF;
    return s;
}



// Optimize ticks->format|precision wrt to legibility.
static void opt_format(VklAxesTicks* ticks, VklAxesContext* ctx)
{
    double l = -INF, best_l = -INF;
    VklTickFormat best_format = VKL_TICK_FORMAT_UNDEFINED;
    uint32_t best_precision = 0;
    for (uint32_t f = 1; f <= 2; f++)
    {
        ticks->format = (VklTickFormat)f;
        for (uint32_t p = 1; p <= PRECISION_MAX; p++)
        {
            ticks->precision = p;
            l = legibility(ticks, ctx);
            if (l > best_l)
            {
                best_format = ticks->format;
                best_precision = p;
                best_l = l;
            }
        }
    }
    if (best_format != VKL_TICK_FORMAT_UNDEFINED)
    {
        ASSERT(best_precision > 0);
        ticks->format = best_format;
        ticks->precision = best_precision;
        // log_debug("%d", duplicate_labels(ticks, ctx));
    }
}



static VklAxesTicks create_ticks(double dmin, double dmax, int32_t m, VklAxesContext ctx)
{
    VklAxesTicks ticks = {0};
    ticks.dmin = dmin;
    ticks.dmax = dmax;

    // Allocate values and labels buffers.
    ticks.values = calloc(MAX_LABELS, sizeof(double));
    ticks.labels = calloc(MAX_LABELS * MAX_GLYPHS_PER_TICK, sizeof(char));

    // Default result.
    ticks.format = VKL_TICK_FORMAT_DECIMAL;
    ticks.precision = 1;
    ticks.value_count_req = (uint32_t)m;
    ticks.value_count = 2;
    ticks.lmin = dmin;
    ticks.lmax = dmax;
    ticks.lstep = dmax - dmin;
    ticks.values[0] = dmin;
    ticks.values[1] = dmax;
    make_labels(&ticks, &ctx, false);

    return ticks;
}



static void debug_ticks(VklAxesTicks* ticks, VklAxesContext* ctx)
{
    log_debug(
        "%f %f %f, n=%d, p=%d, dup %d", ticks->lmin, ticks->lmax, ticks->lstep, ticks->value_count,
        ticks->precision, duplicate_labels(ticks, ctx));
}



/*
k : current number of labels
m : requested number of labels
q : nice number
j : skip, amount among a sequence of nice numbers
z : 10-exponent of the step size
*/
static VklAxesTicks wilk_ext(double dmin, double dmax, int32_t m, VklAxesContext ctx)
{
    ASSERT(dmin < dmax);
    ASSERT(ctx.size_glyph > 0);
    ASSERT(ctx.size_viewport > 0);
    ASSERT(m > 0);

    VklAxesTicks ticks = create_ticks(dmin, dmax, m, ctx);
    if (ctx.size_viewport < 10 * ctx.size_glyph)
    {
        log_debug("degenerate axes context, return a trivial tick range");
        return ticks;
    }

    VklAxesTicks best_ticks = ticks;
    double DEFAULT_Q[] = {1, 5, 2, 2.5, 4, 3};
    dvec4 W = {0.2, 0.25, 0.5, 0.05}; // score weights

    double n = (double)sizeof(DEFAULT_Q) / sizeof(DEFAULT_Q[0]);
    double best_score = -INF;
    Q q = {0};
    q.i = 0;
    q.len = n;
    q.value = DEFAULT_Q[0];
    q.values = DEFAULT_Q;

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

                delta = (dmax - dmin) / (k + 1.) / j / q.value;
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

                        ticks.lmin = lmin;
                        ticks.lmax = lmax;
                        ticks.lstep = lstep;
                        ticks.value_count = tick_count(lmin, lmax, lstep);

                        s = simplicity(q, j, lmin, lmax, lstep);
                        c = coverage(dmin, dmax, lmin, lmax);
                        d = density(k, m, dmin, dmax, lmin, lmax);

                        if (score(W, s, c, d, 1) <= best_score)
                            continue;

                        // The following optimized ticks.format|precision in-place.
                        opt_format(&ticks, &ctx);
                        l = legibility(&ticks, &ctx);

                        scr = score(W, s, c, d, l);
                        if (scr > best_score)
                        {
                            best_score = scr;
                            // Keep track of the best result so far.
                            best_ticks.lmin = lmin;
                            best_ticks.lmax = lmax;
                            best_ticks.lstep = lstep;
                            best_ticks.value_count = tick_count(lmin, lmax, lstep);

                            // Best format and precision are stored in the ticks struct.
                            best_ticks.format = ticks.format;
                            best_ticks.precision = ticks.precision;
                            // debug_ticks(&best_ticks, &ctx);
                        }
                    }
                    z++;
                }
                k++;
            }
        }
        j++;
    }

    best_ticks.value_count = tick_count(best_ticks.lmin, best_ticks.lmax, best_ticks.lstep);
    make_labels(&best_ticks, &ctx, false);
    // debug_ticks(&best_ticks, &ctx);

    return best_ticks;
}



/*************************************************************************************************/
/*  Wrappers                                                                                     */
/*************************************************************************************************/

static VklAxesTicks extend_ticks(VklAxesTicks ticks, VklAxesContext ctx)
{
    VklAxesTicks ex = ticks;
    uint32_t extensions = ctx.extensions;

    // requested range (extended)
    // extended left/right or top/bottom
    double diff = ticks.dmax - ticks.dmin;
    ticks.dmin -= extensions * diff;
    ticks.dmax += extensions * diff;
    // ticks range (extended)
    ex.lmin -= extensions * diff;
    ex.lmax += extensions * diff;
    ex.value_count_req = (2 * extensions + 1) * (uint32_t)ticks.value_count_req;
    ASSERT(ex.value_count_req > 0);

    // Generate the values between lmin and lmax.
    uint32_t n = round(1 + (ticks.lmax - ticks.lmin) / ticks.lstep);
    ASSERT(n >= 2);
    // Extension of the requested range.
    // n is now the total number of ticks across the extended range
    n *= (2 * extensions + 1);

    ex.value_count = n;

    // Generate extended values and labels.
    ASSERT(ex.values != NULL);
    ASSERT(ex.values == ticks.values);
    FREE(ex.values);
    ticks.values = NULL;

    ASSERT(ex.labels != NULL);
    ASSERT(ex.labels == ticks.labels);
    FREE(ex.labels);
    ticks.labels = NULL;
    ex.values = calloc(n, sizeof(double));
    ex.labels = calloc(n * MAX_GLYPHS_PER_TICK, sizeof(char));
    make_labels(&ex, &ctx, true);
    ex.lmin_orig = ticks.lmin;
    ex.lmax_orig = ticks.lmax;
    return ex;
}



static VklAxesTicks vkl_ticks(double dmin, double dmax, VklAxesContext ctx)
{
    ASSERT(dmin < dmax);
    ASSERT(ctx.coord <= VKL_AXES_COORD_Y);
    ASSERT(ctx.size_glyph > 0);
    ASSERT(ctx.size_viewport > 0);

    bool x_axis = ctx.coord == VKL_AXES_COORD_X;

    // NOTE: factor Y because we average 6 characters per tick, and this only counts on the x axis.
    // This number is only an initial guess, the algorithm will find a proper one.
    int32_t label_count_req = (int32_t)ceil(
        ((TARGET_DENSITY * ctx.size_viewport) / ((x_axis ? 6 : 2) * ctx.size_glyph)));
    label_count_req = MAX(2, label_count_req);

    log_debug(
        "running extended Wilkinson algorithm on axis %d with %d labels on range [%.3f, %.3f], "
        "viewport size %.1f, glyph size %.1f, extension %d",
        ctx.coord, label_count_req, dmin, dmax, ctx.size_viewport, ctx.size_glyph, ctx.extensions);
    VklAxesTicks ticks = wilk_ext(dmin, dmax, label_count_req, ctx);
    ASSERT(ticks.lstep > 0);
    ASSERT(ticks.lmin < ticks.lmax);
    ASSERT(ticks.value_count > 0);
    ASSERT(ticks.values != NULL);
    ASSERT(ticks.labels != NULL);

    log_debug(
        "found %d labels, [%.5f, %.5f] with step %.5f", //
        ticks.value_count, ticks.lmin, ticks.lmax, ticks.lstep);
    return extend_ticks(ticks, ctx);
}



static void vkl_ticks_destroy(VklAxesTicks* ticks)
{
    ASSERT(ticks != NULL);
    FREE(ticks->values);
    FREE(ticks->labels);
}



#endif
