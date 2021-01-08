/*
C implementation of the Extended Wilkinson’s Algorithm, see

http://vis.stanford.edu/papers/tick-labels
http://vis.stanford.edu/files/2010-TickLabels-InfoVis.pdf

Implementation adapted from Adam Lucke's at

https://github.com/quantenschaum/ctplot/blob/master/ctplot/ticks.py

*/

#ifndef VKL_EXWILK_HEADER
#define VKL_EXWILK_HEADER

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



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

#define ITER_TICKS                                                                                \
    double x = 0;                                                                                 \
    uint32_t n = floor(1 + (lmax - lmin) / lstep);                                                \
    ASSERT(n >= 2);                                                                               \
    if (n >= 3)                                                                                   \
    {                                                                                             \
        ASSERT(lmin + (n - 1) * lstep <= lmax);                                                   \
        ASSERT(lmin + n * lstep >= lmax);                                                         \
    }                                                                                             \
    for (uint32_t i = 0; i < n; i++)



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Tick format type.
typedef enum
{
    VKL_TICK_FORMAT_UNDEFINED,
    VKL_TICK_FORMAT_DECIMAL,
    VKL_TICK_FORMAT_SCIENTIFIC,
} VklTickFormatType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklTickFormat VklTickFormat;
typedef struct VklAxesContext VklAxesContext;
typedef struct Q Q;
typedef struct R R;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklTickFormat
{
    VklTickFormatType format_type;
    uint32_t precision; // number of digits after the dot
};



struct VklAxesContext
{
    VklAxisCoord coord;
    float size_viewport; // along the current dimension
    float size_glyph;    // either width or height
    char* labels;        // used to store labels to avoid too many allocations during search
};



struct Q
{
    int32_t i;
    double value;

    uint32_t len;
    double* values;
};



struct R
{
    double lmin, lmax, lstep;
    int32_t j, q, k;
    VklTickFormat f;
    double scr;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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



VKY_INLINE double leg(VklTickFormat format, double x)
{
    double ax = fabs(x);
    double l = 0;
    switch (format.format_type)
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

    // TODO: format.precision, penalize larger precisions because of less legibility

    return l;
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



VKY_INLINE double
overlap(VklTickFormat format, double lmin, double lmax, double lstep, VklAxesContext context)
{
    double d = 0;             // distance between label i and i+1
    double min_overlap = INF; //
    double label_overlap = 0; //
    double size = context.size_viewport;
    double glyph = context.size_glyph;
    uint32_t n0 = 1, n1 = 1;

    ITER_TICKS
    {
        if (i == n - 1)
            break;
        x = lmin + i * lstep;
        ASSERT(x <= lmax);

        // NOTE: the size that takes each label on the current coordinate is:
        // - X axis: number of characters in the label times the glyph width
        // - Y axis: always 1 times the glyph height
        if (context.coord == VKL_AXES_COORD_X)
        {
            n0 = strlen(&context.labels[i * MAX_GLYPHS_PER_TICK]);
            n1 = strlen(&context.labels[(i + 1) * MAX_GLYPHS_PER_TICK]);
        }
        // Compute the distance between the current label and the next.
        d = MAX(0, lstep / (lmax - lmin) * size - glyph / 2 * (n0 + n1));

        // Compute the overlap for the current label.
        label_overlap = dist_overlap(d);
        ASSERT(label_overlap <= 1);

        // Compute the minimum overlap between two successive labels.
        if (label_overlap < min_overlap)
            min_overlap = label_overlap;
    }

    // log_debug("overlap %f %f %f, %f", lmin, lmax, lstep, min_overlap);
    return min_overlap;
}



VKY_INLINE void _get_tick_format(VklTickFormat format, char* fmt)
{
    uint32_t offset = 4;
    strcpy(fmt, "%s%.XF"); // [2] = precision, [3] = f or e
    snprintf(&fmt[offset], 4, "%d", format.precision);
    switch (format.format_type)
    {
    case VKL_TICK_FORMAT_DECIMAL:
        fmt[offset + 1] = 'f';
        break;
    case VKL_TICK_FORMAT_SCIENTIFIC:
        fmt[offset + 1] = 'e';
        break;
    default:
        log_error("unknown tick format %d", format.format_type);
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



static void
make_labels(VklTickFormat format, double lmin, double lmax, double lstep, VklAxesContext context)
{
    ASSERT(context.labels != NULL);
    char tick_format[12] = {0};
    _get_tick_format(format, tick_format);

    ITER_TICKS
    {
        x = lmin + i * lstep;
        ASSERT(x <= lmax + .5 * lstep);
        _tick_label(x, tick_format, &context.labels[i * MAX_GLYPHS_PER_TICK]);
    }
}



static double
legibility(VklTickFormat format, double lmin, double lmax, double lstep, VklAxesContext context)
{
    ASSERT(lmin < lmax);
    ASSERT(lstep > 0);

    double f = 0;

    // Format part.
    ITER_TICKS
    {
        x = lmin + i * lstep;
        ASSERT(x <= lmax + .5 * lstep);
        f += leg(format, x);
    }
    f = .9 * f / MAX(1, n); // TODO: 0-extended?

    make_labels(format, lmin, lmax, lstep, context);

    // Overlap part.
    double o = overlap(format, lmin, lmax, lstep, context);

    // Duplicates part.
    double d = 1;
    // TODO: take into account the precision, and penalize states where there are 2 identical
    // labels.

    ASSERT(f <= 1);
    ASSERT(o <= 1);
    ASSERT(d <= 1);

    return (f + o + d) / 3.0;
}



static VklTickFormat opt_format(double lmin, double lmax, double lstep, VklAxesContext context)
{
    double l = -INF, best_l = -INF;
    VklTickFormat format = {0}, best_format = {0};
    for (uint32_t f = 1; f <= 2; f++)
    {
        format.format_type = (VklTickFormatType)f;
        for (uint32_t p = 1; p <= PRECISION_MAX; p++)
        {
            format.precision = p;
            l = legibility(format, lmin, lmax, lstep, context);
            if (l > best_l)
            {
                best_format = format;
                best_l = l;
            }
        }
    }
    ASSERT(best_format.format_type != VKL_TICK_FORMAT_UNDEFINED);
    ASSERT(best_format.precision > 0);
    return best_format;
}



static double
score(dvec4 weights, double simplicity, double coverage, double density, double legibility)
{
    double s = weights[0] * simplicity + weights[1] * coverage + //
               weights[2] * density + weights[3] * legibility;
    // if (s < -100)
    //     s = -INF;
    return s;
}



/*
k : current number of labels
m : requested number of labels
q : nice number
j : skip, amount among a sequence of nice numbers
z : 10-exponent of the step size
*/
static R wilk_ext(double dmin, double dmax, int32_t m, VklAxesContext context)
{
    // if ((dmin >= dmax) || (m < 1)
    // check viewport size
    // context.viewport_size[0] / context.dpi_factor < 200 ||
    // context.viewport_size[1] / context.dpi_factor < 200
    // )
    // {
    //     return (R){dmin, dmax, dmax - dmin, 1, 0, 2, {0, 0, 0}, 0};
    // }

    ASSERT(dmin < dmax);
    ASSERT(context.size_glyph > 0);
    ASSERT(context.size_viewport > 0);
    ASSERT(m > 0);
    if (context.size_viewport < 10 * context.size_glyph)
    {
        log_debug("degenerate axes context, return a trivial tick range");
        return (R){dmin, dmax, dmax - dmin, 1, 0, 2, {VKL_TICK_FORMAT_DECIMAL, 1}, 0};
    }

    context.labels = calloc(MAX_LABELS * MAX_GLYPHS_PER_TICK, sizeof(char));

    double DEFAULT_Q[] = {1, 5, 2, 2.5, 4, 3};
    dvec4 W = {0.2, 0.25, 0.5, 0.05}; // score weights

    double n = (double)sizeof(DEFAULT_Q) / sizeof(DEFAULT_Q[0]);
    double best_score = -INF;
    R result = {0};
    Q q = {0};
    q.i = 0;
    q.len = n;
    q.value = DEFAULT_Q[0];
    q.values = DEFAULT_Q;

    int32_t j = 1;
    int32_t u, k;
    double sm, dm, delta, z, step, cm, min_start, max_start, l, lmin, lmax, lstep, start, s, c, d,
        scr;
    VklTickFormat format;
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
                    // if (step > dmax - dmin)
                    //    break;
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

                        s = simplicity(q, j, lmin, lmax, lstep);
                        c = coverage(dmin, dmax, lmin, lmax);
                        d = density(k, m, dmin, dmax, lmin, lmax);
                        format = opt_format(lmin, lmax, lstep, context);
                        l = legibility(format, lmin, lmax, lstep, context);

                        ASSERT(lstep > 0);
                        scr = score(W, s, c, d, l);
                        // log_debug("score %f: s %f c %f d %f l %f", scr, s, c, d, l);

                        if (scr > best_score)
                        {
                            best_score = scr;
                            result = (R){lmin, lmax, lstep, j, q.value, k, format, scr};
                        }
                    }
                    z++;
                }
                k++;
            }
        }
        j++;
    }
    FREE(context.labels);
    return result;
}



#endif
