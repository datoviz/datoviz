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

#define INF   100000000
#define J_MAX 10
#define K_MAX 50
#define Z_MAX 18

#define VKL_AXES_NORMAL_RANGE(x)                                                                  \
    ((x) == 0 || ((VKL_AXES_DECIMAL_FORMAT_MIN <= (x)) && ((x) < VKL_AXES_DECIMAL_FORMAT_MAX)))

#define VKL_TICK_FORMAT_COUNT 2

#define VKL_AXES_DECIMAL_FORMAT_MIN .01
#define VKL_AXES_DECIMAL_FORMAT_MAX 1000

#define VKL_AXES_COVERAGE_NTICKS_X    2
#define VKL_AXES_COVERAGE_NTICKS_Y    2
#define VKL_AXES_PHYSICAL_DENSITY_MAX .35


/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKL_TICK_FORMAT_DECIMAL,
    VKL_TICK_FORMAT_SCIENTIFIC,
} VklTickFormatType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklTickFormat VklTickFormat;
typedef struct VklAxesTickRange VklAxesTickRange;
typedef struct VklAxesContext VklAxesContext;
typedef struct Q Q;
typedef struct R R;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklTickFormat
{
    VklTickFormatType format_type;
    int32_t precision; // number of digits after the dot
    double legibility;
};



struct VklAxesTickRange
{
    double vmin, vmax, step;
    double vmin_ndc, vmax_ndc, step_ndc;
    VklTickFormat format;
};



struct VklAxesContext
{
    uint8_t coord; // TODO enum
    vec2 glyph_size, viewport_size;
    double dpi_factor;
    bool debug;
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

R wilk_ext(double dmin, double dmax, int32_t m, int32_t only_inside, VklAxesContext context);



static double coverage(double dmin, double dmax, double lmin, double lmax)
{
    return 1. - 0.5 * (pow(dmax - lmax, 2) + pow(dmin - lmin, 2)) / pow(0.1 * (dmax - dmin), 2);
}



static double coverage_max(double dmin, double dmax, double span)
{
    double drange = dmax - dmin;
    if (span > drange)
        return 1. - pow(0.5 * (span - drange), 2) / pow(0.1 * drange, 2);
    else
        return 1.;
}



static double density(double k, double m, double dmin, double dmax, double lmin, double lmax)
{
    double r = (k - 1.) / (lmax - lmin);
    double rt = (m - 1.) / (fmax(lmax, dmax) - fmin(lmin, dmin));
    return 2. - fmax(r / rt, rt / r);
}



static double density_max(int32_t k, int32_t m)
{
    if (k >= m)
        return 2. - (k - 1.0) / (m - 1.0);
    else
        return 1.;
}



static double simplicity(Q q, int32_t j, double lmin, double lmax, double lstep)
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

    // Penalize ticks where 0 is crossed but not on the ticks.
    if (lmin < 0 && lmax > 0 && fmod(lmax / lstep, 1) > eps)
        return -INF;

    return (n - i) / (n - 1.0) + v - j;
}



static double simplicity_max(Q q, int32_t j)
{
    int64_t n = q.len;
    int32_t i = q.i + 1;
    int32_t v = 1;
    return (n - i) / (n - 1.0) + v - j;
}



static int32_t
optimal_precision(double lmin, double lmax, double lstep, VklTickFormatType format_type)
{
    double ax = fmax(fabs(lmin), fabs(lmax));
    assert(ax > 0);
    bool normal_range = VKL_AXES_DECIMAL_FORMAT_MIN <= ax && ax < VKL_AXES_DECIMAL_FORMAT_MAX;
    int32_t precision = 0;
    if (format_type == VKL_TICK_FORMAT_DECIMAL)
    {
        if (lstep >= 1 && normal_range)
            return 1;
        precision = (int)fabs(floor(log10(lstep)));
        precision++;
    }
    else if (format_type == VKL_TICK_FORMAT_SCIENTIFIC)
    {
        // HACK: avoid divide by zero.
        if (fabs(lmin) < 1e-8)
            lmin = 1;
        precision = (int)fabs(floor(log10(lstep / fabs(lmin))));
    }
    if (precision < 0)
    {
        log_warn("precision is negative: %d", precision);
        precision = 0;
    }
    return precision;
}



static double
leg_overlap(double lmin, double lmax, double lstep, VklTickFormat format, VklAxesContext context)
{
    assert(lmin <= lmax);
    assert(lstep > 0);

    int32_t n_labels = 1 + (int)ceil((lmax - lmin) / lstep);
    double glyph_size = context.glyph_size[context.coord];
    double viewport_size = context.viewport_size[context.coord];

    assert(n_labels > 0);
    assert(glyph_size > 0);
    assert(viewport_size > 0);

    int32_t nglyphs = 0;
    if (format.format_type == VKL_TICK_FORMAT_DECIMAL)
    {
        nglyphs = 2 + (int)floor(fabs(log10(fmax(fabs(lmin), fabs(lmax))))) + format.precision;
    }
    else
    {
        nglyphs = 6 + format.precision;
    }
    assert(nglyphs > 0);

    double size = context.coord == 0 ? VKL_AXES_COVERAGE_NTICKS_X * nglyphs * glyph_size
                                     : VKL_AXES_COVERAGE_NTICKS_Y * glyph_size;
    double coverage = size * n_labels / viewport_size;
    if (coverage < VKL_AXES_PHYSICAL_DENSITY_MAX)
    {
        return pow(coverage / VKL_AXES_PHYSICAL_DENSITY_MAX, 2);
    }
    else
        return -100 - pow(coverage, 2);
}



static VklTickFormat legibility(double lmin, double lmax, double lstep, VklAxesContext context)
{
    assert(lstep > 0);
    double ax = fmax(fabs(lmin), fabs(lmax));
    bool normal_range = VKL_AXES_NORMAL_RANGE(ax);

    // Determine the format.
    VklTickFormatType format_type =
        normal_range ? VKL_TICK_FORMAT_DECIMAL : VKL_TICK_FORMAT_SCIENTIFIC;

    // Determine the optimal precision (number of digits after dot).
    int32_t precision = optimal_precision(lmin, lmax, lstep, format_type);
    VklTickFormat format = {format_type, precision, 0};

    // The legibility score is the physical coverage of the labels.
    double score_leg_overlap = leg_overlap(lmin, lmax, lstep, format, context);
    format.legibility = score_leg_overlap;
    return format;
}



static double
score(dvec4 weights, double simplicity, double coverage, double density, double legibility)
{
    double s = weights[0] * simplicity + weights[1] * coverage + //
               weights[2] * density + weights[3] * legibility;
    if (s < -INF / 2)
        s = -INF;
    return s;
}



/*
k : current number of labels
m : requested number of labels
q : nice number
j : skip, amount among a sequence of nice numbers
z : 10-exponent of the step size
*/
R wilk_ext(double dmin, double dmax, int32_t m, int32_t only_inside, VklAxesContext context)
{
    if ((dmin >= dmax) || (m < 1)
        // check viewport size
        // context.viewport_size[0] / context.dpi_factor < 200 ||
        // context.viewport_size[1] / context.dpi_factor < 200
    )
    {
        return (R){dmin, dmax, dmax - dmin, 1, 0, 2, {0, 0, 0}, 0};
    }

    double DEFAULT_Q[] = {1, 5, 2, 2.5, 4, 3};
    dvec4 W = {0.2, 0.25, 0.5, 0.05};

    double n = (double)sizeof(DEFAULT_Q) / sizeof(DEFAULT_Q[0]);
    double best_score = -INF;
    R result = {0};
    Q q = {0};
    q.i = 0;
    q.len = n;
    q.value = DEFAULT_Q[0];
    q.values = DEFAULT_Q;

    int32_t j = 1;
    while (j < J_MAX)
    {
        // printf("j %d\n", j);
        for (int32_t u = 0; u < n; u++)
        {
            // printf("u %d\n", u);
            q.i = u;
            q.value = DEFAULT_Q[q.i];
            double sm = simplicity_max(q, j);

            if (score(W, sm, 1, 1, 1) < best_score)
            {
                j = INF;
                break;
            }

            int32_t k = 2;
            while (k < K_MAX)
            {
                // printf("k %d\n", k);
                double dm = density_max(k, m);

                if (score(W, sm, 1, dm, 1) < best_score)
                    break;

                double delta = (dmax - dmin) / (k + 1.) / j / q.value;
                double z = ceil(log10(delta));

                while (z < Z_MAX)
                {
                    // printf("z %f\n", z);
                    assert(j > 0);
                    assert(q.value > 0);
                    double step = j * q.value * pow(10., z);
                    assert(step > 0);
                    double cm = coverage_max(dmin, dmax, step * (k - 1.));

                    if (score(W, sm, cm, dm, 1) < best_score)
                        break;

                    double min_start = floor(dmax / step) * j - (k - 1.) * j;
                    double max_start = ceil(dmin / step) * j;

                    if (min_start > max_start)
                    {
                        z++;
                        break;
                    }

                    for (double start = min_start; start <= max_start; start++)
                    {
                        double lmin = start * (step / j);
                        double lmax = lmin + step * (k - 1.0);
                        double lstep = step;

                        double s = simplicity(q, j, lmin, lmax, lstep);
                        double c = coverage(dmin, dmax, lmin, lmax);
                        double d = density(k, m, dmin, dmax, lmin, lmax);
                        assert(lstep > 0);
                        VklTickFormat format = legibility(lmin, lmax, lstep, context);
                        double scr = score(W, s, c, d, format.legibility);

                        if ((scr > best_score) &&
                            ((only_inside <= 0) || ((lmin >= dmin) && (lmax <= dmax))) &&
                            ((only_inside >= 0) || ((lmin <= dmin) && (lmax >= dmax))))
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

    context.debug = true;
    // leg_overlap(result.lmin, result.lmax, result.lstep, result.f, context);
    return result;
}



#endif
