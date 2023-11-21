/*************************************************************************************************/
/*  Ticks                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/ticks.h"
#include "_macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
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
#define SCORE_WEIGHTS                                                                             \
    {                                                                                             \
        0.2, 0.25, 0.5, 0.05                                                                      \
    }



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

DVZ_INLINE uint32_t tick_count(double lmin, double lmax, double lstep)
{
    return floor(1 + (lmax - lmin) / lstep);
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



DVZ_INLINE double simplicity(Q q, int32_t j, double lmin, double lmax, double lstep)
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



DVZ_INLINE double simplicity_max(Q q, int32_t j)
{
    int64_t n = q.len;
    int32_t i = q.i + 1;
    int32_t v = 1;
    return (n - i) / (n - 1.0) + v - j;
}



/*************************************************************************************************/
/*  Format                                                                                       */
/*************************************************************************************************/

DVZ_INLINE double leg(DvzTicksFormat format, uint32_t precision, double x)
{
    double ax = fabs(x);
    double l = 0;
    switch (format)
    {
    case DVZ_TICKS_FORMAT_DECIMAL:
        l = ax > 1e-4 && ax < 1e6 ? 1 : 0;
        l = (int)floor(log10(ax)) < -(int)precision ? 0 : l;
        break;

    case DVZ_TICKS_FORMAT_SCIENTIFIC:
        l = .25;
        break;

    default:
        l = 0;
        break;
    }

    return l;
}



static inline double legibility(DvzTicks* ticks)
{
    uint32_t n = 0; // TODO
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

    // // Compute the labels.
    // ticks->lmin_ex = ticks->lmin_in;
    // ticks->lmax_ex = ticks->lmax_in;
    // make_labels(ticks, ctx, false);

    // Overlap part.
    // double o = dist_overlap(min_distance_labels(ticks, ctx));

    // Duplicates part.
    // double d = duplicate_labels(ticks, ctx) ? -INF : 1;

    ASSERT(f <= 1);
    // ASSERT(o <= 1);
    // ASSERT(d <= 1);

    // double out = (f + o + d) / 3.0;
    double out = f;
    if (out < -INF / 10)
        out = -INF;
    return out;
}



/*************************************************************************************************/
/*  Algorithm                                                                                    */
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



// Optimize ticks->format|precision wrt to legibility.
static inline void opt_format(DvzTicks* ticks)
{
    double l = -INF, best_l = -INF;
    DvzTicksFormat best_format = DVZ_TICKS_FORMAT_UNDEFINED;
    uint32_t best_precision = 0;
    for (uint32_t f = 1; f <= 2; f++)
    {
        ticks->format = (DvzTicksFormat)f;
        for (uint32_t p = 1; p <= PRECISION_MAX; p++)
        {
            ticks->precision = p;
            l = legibility(ticks);
            if (l > best_l)
            {
                best_format = ticks->format;
                best_precision = p;
                best_l = l;
            }
        }
    }
    if (best_format != DVZ_TICKS_FORMAT_UNDEFINED)
    {
        ASSERT(best_precision > 0);
        ticks->format = best_format;
        ticks->precision = best_precision;
        // log_debug("%d", duplicate_labels(ticks, ctx));
    }
}



/*
k : current number of labels
m : requested number of labels
q : nice number
j : skip, amount among a sequence of nice numbers
z : 10-exponent of the step size
*/
static bool wilk_ext(DvzTicks* ticks, int32_t m)
{
    ANN(ticks);
    double dmin = ticks->dmin;
    double dmax = ticks->dmax;
    double range_size = ticks->range_size;
    double glyph_size = ticks->glyph_size;

    ASSERT(dmin <= dmax);
    ASSERT(range_size > 0);
    ASSERT(glyph_size > 0);
    ASSERT(m > 0);
    if (range_size < 10 * glyph_size)
    {
        log_debug("degenerate axes context, return a trivial tick range");
        return false;
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

                        ticks->lmin = lmin;
                        ticks->lmax = lmax;
                        ticks->lstep = lstep;

                        s = simplicity(q, j, lmin, lmax, lstep);
                        c = coverage(dmin, dmax, lmin, lmax);
                        d = density(k, m, dmin, dmax, lmin, lmax);

                        if (score(W, s, c, d, 1) <= best_score)
                            continue;

                        // The following optimized ticks.format|precision in-place.
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

                            // Best format and precision are stored in the ticks struct.
                            best_ticks.format = ticks->format;
                            best_ticks.precision = ticks->precision;
                        }
                    }
                    z++;
                }
                k++;
            }
        }
        j++;
    }

    bool has_changed =
        ((best_ticks.lmin != ticks->lmin) ||        //
         (best_ticks.lmax != ticks->lmax) ||        //
         (best_ticks.lstep != ticks->lstep) ||      //
         (best_ticks.format != ticks->format) ||    //
         (best_ticks.precision != ticks->precision) //
        );

    ticks->lmin = best_ticks.lmin;
    ticks->lmax = best_ticks.lmax;
    ticks->lstep = best_ticks.lstep;
    ticks->format = best_ticks.format;
    ticks->precision = best_ticks.precision;

    return has_changed;
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

    int32_t m = (int32_t)requested_count;
    return wilk_ext(ticks, m);
}



// lmin, lmax, lstep, returns the number of ticks
uint32_t dvz_ticks_range(DvzTicks* ticks, dvec3 range)
{
    ANN(ticks);
    range[0] = ticks->lmin;
    range[1] = ticks->lmax;
    range[2] = ticks->lstep;

    // TODO: return number of ticks
    return 0;
}



DvzTicksFormat dvz_ticks_format(DvzTicks* ticks)
{
    // decimal or scientific notation
    ANN(ticks);
    return ticks->format;
}



uint32_t dvz_ticks_precision(DvzTicks* ticks)
{
    // number of digits after the dot
    ANN(ticks);
    return ticks->precision;
}



// return true if the score with that range is significantly lower than the current score
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
        "%.6f -> %.6f (step %.6f), format %d, precision %d", //
        ticks->lmin, ticks->lmax, ticks->lstep, ticks->format, ticks->precision);
}



void dvz_ticks_destroy(DvzTicks* ticks)
{
    ANN(ticks);
    FREE(ticks);
}
