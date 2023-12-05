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
/*  Constants                                                                                    */
/*************************************************************************************************/


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
    DvzLabels* labels, DvzTicksFormat format, uint32_t precision, //
    int32_t exponent, double offset,                              //
    double lmin, double lmax, double lstep)
{
    ANN(labels);

    char tick_format[12] = {0};
    _get_tick_format(format, precision, tick_format);

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
    double exp = pow(10, exponent);
    for (uint32_t i = 0; i < count; i++)
    {
        x = x0 + i * lstep;

        // Take offset and exponent into account.
        if (_is_format_factored(format))
        {
            x -= offset;
            x /= exp;
        }

        s = &labels->labels[k];
        _tick_label(x, tick_format, s);

        n = strnlen(s, MAX_GLYPHS_PER_LABEL * 2);
        ASSERT(n < MAX_GLYPHS_PER_LABEL);

        labels->length[i] = n;
        labels->index[i] = k;
        k += (n + 1); // NOTE: 0-terminated strings
    }

    // Display the exponent.
    if (_is_format_factored(format))
    {
        snprintf(labels->exponent, DVZ_LABELS_MAX_EXPONENT_LENGTH, "1e%d", exponent);
        snprintf(labels->offset, DVZ_LABELS_MAX_OFFSET_LENGTH, "%e", offset);
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
    if (strnlen(labels->exponent, DVZ_LABELS_MAX_EXPONENT_LENGTH) > 0)
    {
        printf("exponent : %s\n", labels->exponent);
        printf("offset   : %s\n", labels->offset);
    }
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
