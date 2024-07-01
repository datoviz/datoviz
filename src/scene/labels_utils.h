/*************************************************************************************************/
/*  Labels utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_LABELS_UTILS
#define DVZ_HEADER_LABELS_UTILS


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_macros.h"
#include "scene/labels.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define IDIV(x) ((int32_t)(x) / pow(10, oom))



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static inline void _get_tick_format(DvzTicksFormat format, uint32_t precision, char* fmt)
{
    uint32_t offset = 4;
    strcpy(fmt, "%s%.XF"); // [2] = precision, [3] = f or e
    snprintf(&fmt[offset], 4, "%u", precision);
    switch (format)
    {
    case DVZ_TICKS_FORMAT_DECIMAL:
    case DVZ_TICKS_FORMAT_DECIMAL_FACTORED:
    case DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED:
        fmt[offset + 1] = 'f';
        break;
    case DVZ_TICKS_FORMAT_SCIENTIFIC:
        fmt[offset + 1] = 'e';
        break;
    default:
        log_error("unknown tick format %d", format);
        break;
    }
}



static inline void _tick_label(double x, char* tick_format, char* out)
{
    if (x == 0)
    {
        snprintf(out, 2, "0");
        return;
    }
    char sign[2] = {0};
    sign[0] = x < 0 ? '-' : '+';
    snprintf(out, MAX_GLYPHS_PER_LABEL, tick_format, sign, fabs(x));
    ASSERT(strnlen(out, 2 * MAX_GLYPHS_PER_LABEL) < MAX_GLYPHS_PER_LABEL);
}



static inline bool _is_format_factored(DvzTicksFormat format)
{
    switch (format)
    {
    case DVZ_TICKS_FORMAT_DECIMAL_FACTORED:
    case DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED:
    case DVZ_TICKS_FORMAT_MILLIONS_FACTORED:
    case DVZ_TICKS_FORMAT_THOUSANDS_FACTORED:
        return true;
    default:
        return false;
    }
    return false;
}



static void
_find_exponent_offset(double lmin, double lmax, int32_t* out_exponent, double* out_offset)
{
    // Loose translation of:
    // https://github.com/matplotlib/matplotlib/blob/main/lib/matplotlib/ticker.py#L708

    if (lmin == lmax)
        return;

    // # min, max comparing absolute values (we want division to round towards
    // # zero so we work on absolute values).
    // abs_min, abs_max = sorted([abs(float(lmin)), abs(float(lmax))])
    double abs_min = fabs(lmin);
    double abs_max = fabs(lmax);
    abs_min = MIN(abs_min, abs_max);
    abs_max = MAX(abs_min, abs_max);

    // sign = math.copysign(1, lmin)
    double sign = copysign(1, lmin);

    // # What is the smallest power of ten such that abs_min and abs_max are
    // # equal up to that precision?
    // # Note: Internally using oom instead of 10 ** oom avoids some numerical
    // # accuracy issues.
    // oom_max = np.ceil(math.log10(abs_max))
    // oom = 1 + next(oom for oom in itertools.count(oom_max, -1)
    //                if abs_min // 10 ** oom != abs_max // 10 ** oom)
    int32_t oom_max = (int32_t)ceil(log10(abs_max));

    int32_t oom = oom_max;
    for (; oom >= 0; oom--)
    {
        if (IDIV(abs_min) != IDIV(abs_max))
            break;
    }
    oom += 1;

    // if (abs_max - abs_min) / 10 ** oom <= 1e-2:
    //     # Handle the case of straddling a multiple of a large power of ten
    //     # (relative to the span).
    if ((abs_max - abs_min) / pow(10, oom) <= 1e-2)
    {
        // # What is the smallest power of ten such that abs_min and abs_max
        // # are no more than 1 apart at that precision?
        // oom = 1 + next(oom for oom in itertools.count(oom_max, -1)
        //                if abs_max // 10 ** oom - abs_min // 10 ** oom > 1)
        oom = oom_max;
        for (; oom >= 0; oom--)
        {
            if ((IDIV(abs_max) - IDIV(abs_min)) > 1)
                break;
        }
        oom += 1;
    }

    // see eg: https://github.com/matplotlib/matplotlib/issues/7104
    int32_t offset_threshold = 4;

    // # Only use offset if it saves at least _offset_threshold digits.
    // n = self._offset_threshold - 1
    int32_t n = offset_threshold - 1;

    // self.offset = (sign * (abs_max // 10 ** oom) * 10 ** oom
    //                if abs_max // 10 ** oom >= 10**n
    //                else 0)
    double offset = IDIV(abs_max) >= pow(10, n) ? sign * (IDIV(abs_max) * pow(10, oom)) : 0;

    // if self.offset:
    //     oom = math.floor(math.log10(vmax - vmin))
    if (offset)
    {
        // NOTE: this should be dmax and dmin but we don't have them in this function
        // at the moment. TODO FIXME?
        oom = floor(log10(lmax - lmin));
    }
    // else:
    //     val = locs.max()
    //     if val == 0:
    //         oom = 0
    //     else:
    //         oom = math.floor(math.log10(val))
    else
    {
        double val = lmax; // TODO: should be dmax?
        if (val == 0)
        {
            oom = 0;
        }
        else
        {
            oom = floor(log10(val));
        }
    }

    *out_offset = offset;
    *out_exponent = oom;

    // if (almin == 0)
    // {
    //     return;
    // }

    // if ((almin / almax) > threshold)
    // {
    //     *offset = lmin;
    // }
    // else
    // {
    //     *offset = 0;
    // }

    // almin = 0;
    // almax -= almin;
    // ASSERT(almax > 0);
    // double l = log10(almax);
    // // NOTE: symmetric version of floor around 0.
    // l = copysign(1, l) * floor(fabs(l));

    // *exponent = (int32_t)l;
}



static inline uint32_t format_label(DvzLabelFormat* fmt, double value, char* out)
{
    ANN(fmt);
    ANN(out);

    int32_t exponent = fmt->exponent;
    double exp = pow(10, exponent);

    // Take offset and exponent into account.
    if (_is_format_factored(fmt->format))
    {
        value -= fmt->offset;
        value /= exp;
    }

    _tick_label(value, fmt->tick_format, out);

    uint32_t n = strnlen(out, MAX_GLYPHS_PER_LABEL * 2);
    ASSERT(n < MAX_GLYPHS_PER_LABEL);

    return n;
}



#endif
