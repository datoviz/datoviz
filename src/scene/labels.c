/*************************************************************************************************/
/*  Labels                                                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/labels.h"
#include "_macros.h"
#include "scene/ticks.h"



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



void _find_exponent_offset(double lmin, double lmax, int32_t* out_exponent, double* out_offset)
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



/*************************************************************************************************/
/*  Label format                                                                                 */
/*************************************************************************************************/

DvzLabelFormat
dvz_label_format(DvzTicksFormat format, uint32_t precision, int32_t exponent, double offset)
{
    DvzLabelFormat f = {
        .format = format,
        .precision = precision,
        .exponent = exponent,
        .offset = offset,
    };
    _get_tick_format(format, precision, f.tick_format);
    return f;
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



/*************************************************************************************************/
/*  Labels functions                                                                             */
/*************************************************************************************************/

DvzLabels* dvz_labels(void)
{
    DvzLabels* labels = (DvzLabels*)calloc(1, sizeof(DvzLabels));

    // NOTE: each label is 0-terminated.
    labels->labels = (char*)calloc(MAX_LABELS * MAX_GLYPHS_PER_LABEL, sizeof(char));
    labels->index = (uint32_t*)calloc(MAX_LABELS, sizeof(uint32_t));
    labels->length = (uint32_t*)calloc(MAX_LABELS, sizeof(uint32_t));

    return labels;
}



uint32_t dvz_labels_generate(
    DvzLabels* labels,
    // label format
    DvzTicksFormat format, uint32_t precision, int32_t exponent, double offset, //
    // ticks positions spec
    double lmin, double lmax, double lstep)
{
    ANN(labels);

    DvzLabelFormat fmt = dvz_label_format(format, precision, exponent, offset);

    ASSERT(lstep > 0);
    double x0 = lmin;
    double x = x0;
    uint32_t count = tick_count(lmin, lmax, lstep);
    labels->count = count;
    if (count == 0)
        return 0;
    ASSERT(count > 0);

    char* s = NULL;
    uint32_t k = 0, n = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        x = x0 + i * lstep;
        s = &labels->labels[k];

        // Generate the label string.
        n = format_label(&fmt, x, s);

        labels->length[i] = n;
        labels->index[i] = k;
        k += (n + 1); // NOTE: 0-terminated strings
    }

    // Display the exponent.
    if (_is_format_factored(format))
    {
        // Exponent.

        if (exponent != 0)
            _tick_label(pow(10, exponent), "x%g", labels->exponent);
        else
            labels->exponent[0] = 0;

        // Offset.
        if (offset != 0)
            _tick_label(offset, fmt.tick_format, labels->offset);
        else
            labels->offset[0] = 0;
    }

    return count;
}



// the output must not be freed
char* dvz_labels_string(DvzLabels* labels)
{
    ANN(labels);
    // concatenation of all null-terminated strings
    return labels->labels;
}



// the output must not be freed
uint32_t* dvz_labels_index(DvzLabels* labels)
{
    ANN(labels);
    // the size is the output of generate(), each number is the index of the first
    // character of that tick
    return labels->index;
}



// the output must not be freed
uint32_t* dvz_labels_length(DvzLabels* labels)
{
    // the size is the number of ticks, each number is the length of the label
    ANN(labels);
    return labels->length;
}



// the output must not be freed
char* dvz_labels_exponent(DvzLabels* labels)
{
    ANN(labels);
    return labels->exponent;
}



// the output must not be freed
char* dvz_labels_offset(DvzLabels* labels)
{
    ANN(labels);
    return labels->offset;
}



void dvz_labels_print(DvzLabels* labels)
{
    ANN(labels);
    for (uint32_t i = 0; i < labels->count; i++)
    {
        printf("%02d\t%s\n", i, &labels->labels[labels->index[i]]);
    }
    printf("\n");

    // Display the exponent if there is one.
    if (labels->exponent[0])
        printf("exponent : %s\n", labels->exponent);

    // Display the offset if there is one.
    if (labels->offset[0])
        printf("offset   : %s\n", labels->offset);
}



void dvz_labels_destroy(DvzLabels* labels)
{
    ANN(labels);

    if (labels->labels)
    {
        FREE(labels->labels);
    }

    if (labels->index)
    {
        FREE(labels->index);
    }

    if (labels->length)
    {
        FREE(labels->length);
    }

    FREE(labels);
}
